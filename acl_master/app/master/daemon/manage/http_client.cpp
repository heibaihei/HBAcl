/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 14 Jun 2017 12:23:36 PM CST
 */

#include "stdafx.h"
#include "action/service_list.h"
#include "action/service_stat.h"
#include "action/service_start.h"
#include "action/service_kill.h"
#include "action/service_stop.h"
#include "action/service_reload.h"
#include "action/service_restart.h"
#include "http_client.h"

http_client::http_client(acl::aio_socket_stream *client, int rw_timeout)
	: conn_(client->get_astream())
	, rw_timeout_(rw_timeout)
	, content_length_(0)
{
	hdr_req_ = NULL;
	req_     = NULL;
	acl_aio_add_close_hook(conn_, on_close, this);
	acl_aio_add_timeo_hook(conn_, on_timeo, this);
}

http_client::~http_client(void)
{
	if (req_)
		http_req_free(req_);
	else if (hdr_req_)
		http_hdr_req_free(hdr_req_);
}

void http_client::reset(void)
{
	if (req_)
	{
		http_req_free(req_);
		req_ = NULL;
	}
	else if (hdr_req_)
		http_hdr_req_free(hdr_req_);

	hdr_req_ = http_hdr_req_new();
	json_.reset();
}

void http_client::wait(void)
{
	reset();

	http_hdr_req_get_async(hdr_req_, conn_, on_head, this, rw_timeout_);
}

int http_client::on_close(ACL_ASTREAM*, void* ctx)
{
	http_client* hc = (http_client*) ctx;
	delete hc;
	return 0;
}

int http_client::on_timeo(ACL_ASTREAM*, void*)
{
	// return -1 so the connection can be closed
	return -1;
}

int http_client::on_head(int status, void* ctx)
{
	http_client* hc = (http_client*) ctx;

	acl_aio_disable_readwrite(hc->conn_);

	if (status != HTTP_CHAT_OK)
	{
		logger_error("invalid status=%d", status);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	if (http_hdr_req_parse(hc->hdr_req_) < 0)
	{
		logger_error("parse http header error");
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	hc->content_length_ = hc->hdr_req_->hdr.content_length;
	if (hc->content_length_ <= 0)
		return hc->handle() ? 0 : -1;

	hc->req_ = http_req_new(hc->hdr_req_);
	http_req_body_get_async(hc->req_, hc->conn_, on_body,
		hc, hc->rw_timeout_);

	return 0;
}

int http_client::on_body(int status, char *data, int dlen, void *ctx)
{
	http_client* hc = (http_client*) ctx;

	if (status >= HTTP_CHAT_ERR_MIN)
	{
		logger_error("status=%d", status);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	if (dlen <= 0)
	{
		logger_error("invalid dlen=%d", dlen);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	hc->json_.update(data);

	if (status == HTTP_CHAT_OK)
	{
		acl_aio_disable_readwrite(hc->conn_);
		return hc->handle() ? 0 : -1;
	}

	return 0;
}

void http_client::do_reply(int status, const acl::string& body)
{
	HTTP_HDR_RES* hdr_res = http_hdr_res_static(status);
	http_hdr_set_keepalive(hdr_req_, hdr_res);
	http_hdr_put_str(&hdr_res->hdr, "Content-Type", "text/json");
	http_hdr_put_int(&hdr_res->hdr, "Content-Length", (int) body.size());

	acl::string buf(body.size() + 256);
	http_hdr_build(&hdr_res->hdr, buf.vstring());
	http_hdr_res_free(hdr_res);
	buf.append(body);

	logger(">>reply: [%s]\r\n", buf.c_str());
	acl_aio_writen(conn_, buf.c_str(), (int) buf.size());
}

bool http_client::handle(void)
{
	const char* cmd = http_hdr_req_param(hdr_req_, "cmd");
	if (cmd == NULL || *cmd == 0)
	{
		//logger_error("cmd null");
		acl::string dummy;
		do_reply(400, dummy);
		if (hdr_req_->hdr.keep_alive)
			wait();
		else
			acl_aio_iocp_close(conn_);
		return true;
	}

#define EQ !strcasecmp

	bool ret;

	if (EQ(cmd, "list"))
		ret = handle_list();
	else if (EQ(cmd, "stat"))
		ret = handle_stat();
	else if (EQ(cmd, "start"))
		ret = handle_start();
	else if (EQ(cmd, "kill"))
		ret = handle_kill();
	else if (EQ(cmd, "stop"))
		ret = handle_stop();
	else if (EQ(cmd, "reload"))
		ret = handle_reload();
	else if (EQ(cmd, "restart"))
		ret = handle_restart();
	else {
		logger_warn("invalid cmd=%s", cmd);
		acl::string dummy;
		do_reply(400, dummy);
		if (hdr_req_->hdr.keep_alive)
			wait();
		else
			acl_aio_iocp_close(conn_);
		return true;
	}

	if (ret && hdr_req_->hdr.keep_alive)
	{
		wait();
		return true;
	}
	else
	{
		acl_aio_iocp_close(conn_);
		return false;
	}
}

bool http_client::handle_list(void)
{
	list_req_t req;
	list_res_t res;

	if (deserialize<list_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<list_res_t>(res.status, res);
		return false;
	}

	service_list service;
	if (service.run(req, res))
	{
		res.status = 200;
		res.msg    = "ok";
	}
	else
	{
		res.status = 500;
		res.msg    = "error";
	}

	reply<list_res_t>(res.status, res);
	return true;
}

bool http_client::handle_stat(void)
{
	stat_req_t req;
	stat_res_t res;

	if (deserialize<stat_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<stat_res_t>(res.status, res);
		return false;
	}

	service_stat service;
	service.run(req, res);
	reply<stat_res_t>(res.status, res);

	return true;
}

bool http_client::handle_kill(void)
{
	kill_req_t req;
	kill_res_t res;

	if (deserialize<kill_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<kill_res_t>(res.status, res);
		return false;
	}

	service_kill service;
	service.run(req, res);
	reply<kill_res_t>(res.status, res);

	return true;
}

bool http_client::handle_stop(void)
{
	stop_req_t req;
	stop_res_t res;

	if (deserialize<stop_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<stop_res_t>(res.status, res);
		return false;
	}

	service_stop service;
	service.run(req, res);
	reply<stop_res_t>(res.status, res);

	return true;
}

bool http_client::handle_start(void)
{
	start_req_t req;
	start_res_t res;

	if (deserialize<start_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<start_res_t>(res.status, res);
		return false;
	}

	service_start service;
	service.run(req, res);
	reply<start_res_t>(res.status, res);

	return true;
}

bool http_client::handle_restart(void)
{
	restart_req_t req;
	restart_res_t res;

	if (deserialize<restart_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<restart_res_t>(res.status, res);
		return false;
	}

	service_restart service;
	service.run(req, res);
	reply<restart_res_t>(res.status, res);

	return true;
}

bool http_client::handle_reload(void)
{
	reload_req_t req;
	reload_res_t res;

	if (deserialize<reload_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<reload_res_t>(res.status, res);
		return false;
	}

	service_reload service;
	service.run(req, res);
	reply<reload_res_t>(res.status, res);

	return true;
}
