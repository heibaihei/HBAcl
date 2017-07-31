#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/master/master_trigger.hpp"
#endif

namespace acl
{

master_trigger::master_trigger() {}

master_trigger::~master_trigger() {}

static bool has_called = false;

void master_trigger::run_daemon(int argc, char** argv)
{
#ifdef ACL_WINDOWS
	logger_fatal("not support ACL_WINDOWS!");
#else
	// ÿ������ֻ����һ��ʵ��������
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	acl_trigger_server_main(argc, argv, service_main,
		ACL_MASTER_SERVER_CTX, this,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_SIGHUP, service_on_sighup,
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		0);
#endif
}

void master_trigger::run_alone(const char* path /* = NULL */,
	int count /* = 1 */, int interval /* = 1 */)
{
	// ÿ������ֻ����һ��ʵ��������
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
#ifdef ACL_WINDOWS
	acl_cpp_init();
#endif
	if (interval <= 0)
		interval = 1;

	// ��ʼ�����ò���
	conf_.load(path);

	service_pre_jail(this);
	service_init(this);

	int   i = 0;
	while (true)
	{
		sleep(interval);
		service_main(this);
		if (count > 0 && ++i >= count)
			break;
	}

	service_exit(this);
}

const char* master_trigger::get_conf_path(void) const
{
#ifndef ACL_WINDOWS
	if (daemon_mode_)
	{
		const char* ptr = acl_trigger_server_conf();
		return ptr && *ptr ? ptr : NULL;
	}
	else
#endif
		return conf_.get_path();
}

//////////////////////////////////////////////////////////////////////////

void master_trigger::service_main(void* ctx)
{
	master_trigger* mt = (master_trigger *) ctx;
	acl_assert(mt != NULL);

#ifndef	ACL_WINDOWS
	if (mt->daemon_mode_)
		acl_watchdog_pat();
#endif
	mt->on_trigger();
}

void master_trigger::service_pre_jail(void* ctx)
{
	master_trigger* mt = (master_trigger *) ctx;
	acl_assert(mt != NULL);

#ifndef ACL_WINDOWS
	if (mt->daemon_mode())
	{
		ACL_EVENT* eventp = acl_trigger_server_event();
		mt->set_event(eventp);  // ���û�����¼�������
	}
#endif

	mt->proc_pre_jail();
}

void master_trigger::service_init(void* ctx)
{
	master_trigger* mt = (master_trigger *) ctx;
	acl_assert(mt != NULL);

	mt->proc_inited_ = true;
	mt->proc_on_init();
}

void master_trigger::service_exit(void* ctx)
{
	master_trigger* mt = (master_trigger *) ctx;
	acl_assert(mt != NULL);

	mt->proc_on_exit();
}

void master_trigger::service_on_sighup(void* ctx)
{
	master_trigger* mt = (master_trigger *) ctx;
	acl_assert(mt);
	mt->proc_on_sighup();
}

}  // namespace acl