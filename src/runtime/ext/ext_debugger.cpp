/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include <runtime/ext/ext_debugger.h>
#include <runtime/ext/ext_string.h>
#include <runtime/eval/debugger/cmd/cmd_user.h>
#include <runtime/eval/debugger/cmd/cmd_interrupt.h>
#include <tbb/concurrent_hash_map.h>
#include <util/logger.h>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

using namespace Eval;
using namespace boost;

const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_FILENAMES =
  DebuggerClient::AutoCompleteFileNames;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_VARIABLES =
  DebuggerClient::AutoCompleteVariables;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CONSTANTS =
  DebuggerClient::AutoCompleteConstants;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CLASSES   =
  DebuggerClient::AutoCompleteClasses;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_FUNCTIONS =
  DebuggerClient::AutoCompleteFunctions;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CLASS_METHODS =
  DebuggerClient::AutoCompleteClassMethods;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CLASS_PROPERTIES =
  DebuggerClient::AutoCompleteClassProperties;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CLASS_CONSTANTS =
  DebuggerClient::AutoCompleteClassConstants;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_KEYWORDS =
  DebuggerClient::AutoCompleteKeyword;
const int64 q_DebuggerClientCmdUser$$AUTO_COMPLETE_CODE =
  DebuggerClient::AutoCompleteCode;

///////////////////////////////////////////////////////////////////////////////

bool f_hphpd_install_user_command(CStrRef cmd, CStrRef clsname) {
  return CmdUser::InstallCommand(cmd, clsname);
}

Array f_hphpd_get_user_commands() {
  return CmdUser::GetCommands();
}

void f_hphpd_break(bool condition /* = true */) {
  if (RuntimeOption::EnableDebugger && condition) {
    ThreadInfo *ti = ThreadInfo::s_threadInfo.getNoCheck();
    FrameInjection *frame = FrameInjection::GetStackFrame(1);
    if (frame && ti->m_reqInjectionData.debugger) {
      Eval::InterruptSite site(frame);
      Eval::Debugger::InterruptHard(site);
    }
  }
}

// TODO: cleanup, maybe in Extension::ShutdownModules
typedef tbb::concurrent_hash_map<StringData*, DebuggerClient*,
                                 StringDataHashICompare> DbgCltMap;
static DbgCltMap s_dbgCltMap;

// FIXME: For now it assumes only one active thread from UI can call this
// function and use the client. If that assumption doesn't hold, we need to
// figure out someway to deal with concurrent accesses.

Variant f_hphpd_get_client(CStrRef name /* = null */) {
  DebuggerClient *client = NULL;
  StringData* sd = new StringData(name.data(), name.size(), CopyString);
  {
    DbgCltMap::accessor acc;
    if (s_dbgCltMap.insert(acc, sd)) {
      client = new DebuggerClient();
      acc->second = client;
    } else {
      free(sd);
      client = acc->second;
    }
  }
  p_DebuggerClient clt(NEWOBJ(c_DebuggerClient));
  clt->m_client = client;
  return clt;
}

///////////////////////////////////////////////////////////////////////////////

c_DebuggerProxyCmdUser::c_DebuggerProxyCmdUser(const ObjectStaticCallbacks *cb) : ExtObjectData(cb) {
}

c_DebuggerProxyCmdUser::~c_DebuggerProxyCmdUser() {
}

void c_DebuggerProxyCmdUser::t___construct() {
}

bool c_DebuggerProxyCmdUser::t_islocal() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerProxyCmdUser, DebuggerProxyCmdUser::islocal);
  return m_proxy->isLocal();
}

Variant c_DebuggerProxyCmdUser::t_send(CObjRef cmd) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerProxyCmdUser, DebuggerProxyCmdUser::send);
  CmdUser cmdUser(cmd);
  return m_proxy->send(&cmdUser);
}

Variant c_DebuggerProxyCmdUser::t___destruct() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerProxyCmdUser, DebuggerProxyCmdUser::__destruct);
  return null;
}

///////////////////////////////////////////////////////////////////////////////

c_DebuggerClientCmdUser::c_DebuggerClientCmdUser(const ObjectStaticCallbacks *cb) : ExtObjectData(cb) {
}

c_DebuggerClientCmdUser::~c_DebuggerClientCmdUser() {
}

void c_DebuggerClientCmdUser::t___construct() {
}

void c_DebuggerClientCmdUser::t_quit() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::quit);
  m_client->quit();
}

static String format_string(DebuggerClient *client,
                            int _argc, CStrRef format, CArrRef _argv) {
  Variant ret = f_sprintf(_argc, format, _argv);
  if (ret.isString()) {
    return ret;
  }
  client->error("Debugger extension failed to format string: %s",
                 format.data());
  return "";
}

void c_DebuggerClientCmdUser::t_print(int _argc, CStrRef format,
                               CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::print);
  m_client->print(format_string(m_client, _argc, format, _argv));
}

void c_DebuggerClientCmdUser::t_help(int _argc, CStrRef format,
                              CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::help);
  m_client->help(format_string(m_client, _argc, format, _argv));
}

void c_DebuggerClientCmdUser::t_info(int _argc, CStrRef format,
                              CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::info);
  m_client->info(format_string(m_client, _argc, format, _argv));
}

void c_DebuggerClientCmdUser::t_output(int _argc, CStrRef format,
                                CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::output);
  m_client->output(format_string(m_client, _argc, format, _argv));
}

void c_DebuggerClientCmdUser::t_error(int _argc, CStrRef format,
                               CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::error);
  m_client->error(format_string(m_client, _argc, format, _argv));
}

void c_DebuggerClientCmdUser::t_code(CStrRef source, int highlight_line /* = 0 */,
                              int start_line_no /* = 0 */,
                              int end_line_no /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::code);
  m_client->code(source, highlight_line, start_line_no, end_line_no);
}

Variant c_DebuggerClientCmdUser::t_ask(int _argc, CStrRef format,
                                CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::ask);
  String ret = format_string(m_client, _argc, format, _argv);
  return String::FromChar(m_client->ask("%s", ret.data()));
}

String c_DebuggerClientCmdUser::t_wrap(CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::wrap);
  return m_client->wrap(str.data());
}

void c_DebuggerClientCmdUser::t_helptitle(CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::helptitle);
  m_client->helpTitle(str.data());
}

void c_DebuggerClientCmdUser::t_helpcmds(int _argc, CStrRef cmd, CStrRef desc,
                                  CArrRef _argv /* = null_array */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::helpcmds);
  std::vector<String> holders;
  std::vector<const char *> cmds;
  cmds.push_back(cmd.data());
  cmds.push_back(desc.data());
  for (int i = 0; i < _argv.size(); i++) {
    String s = _argv[i].toString();
    holders.push_back(s);
    cmds.push_back(s.data());
  }
  m_client->helpCmds(cmds);
}

void c_DebuggerClientCmdUser::t_helpbody(CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::helpbody);
  m_client->helpBody(str.data());
}

void c_DebuggerClientCmdUser::t_helpsection(CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::helpsection);
  m_client->helpSection(str.data());
}

void c_DebuggerClientCmdUser::t_tutorial(CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::tutorial);
  m_client->tutorial(str.data());
}

String c_DebuggerClientCmdUser::t_getcode() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::getcode);
  return m_client->getCode();
}

String c_DebuggerClientCmdUser::t_getcommand() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::getcommand);
  return m_client->getCommand();
}

bool c_DebuggerClientCmdUser::t_arg(int index, CStrRef str) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::arg);
  return m_client->arg(index + 1, str.data());
}

int c_DebuggerClientCmdUser::t_argcount() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::argcount);
  return m_client->argCount() - 1;
}

String c_DebuggerClientCmdUser::t_argvalue(int index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::argvalue);
  return m_client->argValue(index + 1);
}

String c_DebuggerClientCmdUser::t_argrest(int index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::argrest);
  return m_client->argRest(index + 1);
}

Array c_DebuggerClientCmdUser::t_args() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::args);
  StringVec *args = m_client->args();
  Array ret(Array::Create());
  for (unsigned int i = 1; i < args->size(); i++) {
    ret.append(String(args->at(i)));
  }
  return ret;
}

Variant c_DebuggerClientCmdUser::t_send(CObjRef cmd) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::send);
  CmdUser cmdUser(cmd);
  m_client->send(&cmdUser);
  return true;
}

Variant c_DebuggerClientCmdUser::t_xend(CObjRef cmd) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::xend);
  CmdUser cmdUser(cmd);
  CmdUserPtr ret = m_client->xend<CmdUser>(&cmdUser);
  return ret->getUserCommand();
}

Variant c_DebuggerClientCmdUser::t_getcurrentlocation() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::getcurrentlocation);
  BreakPointInfoPtr bpi = m_client->getCurrentLocation();
  Array ret(Array::Create());
  if (bpi) {
    ret.set("file",      String(bpi->m_file));
    ret.set("line",      (int64)bpi->m_line1);
    ret.set("namespace", String(bpi->getNamespace()));
    ret.set("class",     String(bpi->getClass()));
    ret.set("function",  String(bpi->getFunction()));
    ret.set("text",      String(bpi->site()));
  }
  return ret;
}

Variant c_DebuggerClientCmdUser::t_getstacktrace() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::getstacktrace);
  return m_client->getStackTrace();
}

int c_DebuggerClientCmdUser::t_getframe() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::getframe);
  return m_client->getFrame();
}

void c_DebuggerClientCmdUser::t_printframe(int index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::printframe);
  m_client->printFrame(index, m_client->getStackTrace()[index]);
}

void c_DebuggerClientCmdUser::t_addcompletion(CVarRef list) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::addcompletion);
  if (list.isInteger()) {
    m_client->addCompletion((DebuggerClient::AutoComplete)list.toInt64());
  } else {
    Array arr = list.toArray(); // handles string, array and iterators
    std::vector<String> items;
    for (ArrayIter iter(arr); iter; ++iter) {
      items.push_back(iter.second().toString());
    }
    m_client->addCompletion(items);
  }
}

Variant c_DebuggerClientCmdUser::t___destruct() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClientCmdUser, DebuggerClientCmdUser::__destruct);
  return null;
}

///////////////////////////////////////////////////////////////////////////////

const int64 q_DebuggerClient$$STATE_INVALID = -1;
const int64 q_DebuggerClient$$STATE_UNINIT
  = DebuggerClient::StateUninit;
const int64 q_DebuggerClient$$STATE_INITIALIZING
  = DebuggerClient::StateInitializing;
const int64 q_DebuggerClient$$STATE_READY_FOR_COMMAND
  = DebuggerClient::StateReadyForCommand;
const int64 q_DebuggerClient$$STATE_BUSY
  = DebuggerClient::StateBusy;

c_DebuggerClient::c_DebuggerClient(const ObjectStaticCallbacks *cb) : ExtObjectData(cb) {
  m_client = NULL;
}

c_DebuggerClient::~c_DebuggerClient() {
}

void c_DebuggerClient::t___construct() {
}

int64 c_DebuggerClient::t_getstate() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClient, DebuggerClient::getstate);
  if (!m_client) {
    return q_DebuggerClient$$STATE_INVALID;
  }
  return m_client->getClientState();
}

Variant c_DebuggerClient::t_init(CVarRef options) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClient, DebuggerClient::init);
  if (!m_client) {
    raise_warning("invalid client");
    return false;
  }
  if (m_client->getClientState() != DebuggerClient::StateUninit) {
    return m_client->getClientState() == DebuggerClient::StateReadyForCommand;
  }
  if (!options.isArray()) {
    raise_warning("options must be an array");
    return false;
  }
  m_client->setClientState(DebuggerClient::StateInitializing);

  DebuggerClientOptions ops;
  ops.apiMode = true;

  Array opsArr = options.toArray();
  if (opsArr.exists("configFName")) {
    ops.configFName = opsArr.rvalAtRef("configFName").toString().data();
  } else {
    raise_warning("must specify config file name (configFName) in options");
    return false;
  }

  FILE *f = fopen(ops.configFName.c_str(), "r");
  if (!f) {
    raise_warning("cannot access config file %s", ops.configFName.c_str());
    return false;
  }
  fclose(f);

  if (opsArr.exists("host")) {
    ops.host = opsArr.rvalAtRef("host").toString().data();
  }
  if (opsArr.exists("port")) {
    ops.port = opsArr.rvalAtRef("configFName").toInt32();
  }
  if (opsArr.exists("user")) {
    ops.user = opsArr.rvalAtRef("user").toString().data();
  }
  if (opsArr.exists("sandbox")) {
    ops.sandbox = opsArr.rvalAtRef("sandbox").toString().data();
  }

  m_client->init(ops);

  if (ops.host.empty()) {
    ops.host = "localhost";
  }
  if (ops.port < 0) {
    ops.port = RuntimeOption::DebuggerServerPort;
  }
  bool ret = m_client->connect(ops.host, ops.port);
  if (!ret) {
    raise_warning("failed to connect to hphpi %s:%d", ops.host.c_str(),
                  ops.port);
    return false;
  }

  // To wait for the session start interrupt
  DebuggerCommandPtr cmd = m_client->waitForNextInterrupt();
  if (!cmd->is(DebuggerCommand::KindOfInterrupt) ||
      dynamic_pointer_cast<CmdInterrupt>(cmd)->getInterruptType() !=
      SessionStarted) {
    raise_warning("failed to load sandbox");
    return false;
  }

  ret = m_client->initializeMachine();
  if (!ret) {
    raise_warning("failed to initialize machine info");
    return false;
  }

  // To wait for the machine loading sandbox
  cmd = m_client->waitForNextInterrupt();
  if (!cmd->is(DebuggerCommand::KindOfInterrupt) ||
      dynamic_pointer_cast<CmdInterrupt>(cmd)->getInterruptType() !=
      SessionStarted) {
    raise_warning("failed to load sandbox");
    return false;
  }

  m_client->setClientState(DebuggerClient::StateReadyForCommand);

  return true;
}

Variant c_DebuggerClient::t_processcmd(CVarRef cmdName, CVarRef args) {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClient, DebuggerClient::processcmd);
  if (!m_client ||
      m_client->getClientState() < DebuggerClient::StateReadyForCommand) {
    raise_warning("client is not initialized");
    return null;
  }
  if (m_client->getClientState() != DebuggerClient::StateReadyForCommand) {
    raise_warning("client is not ready to take command");
    return null;
  }
  if (!cmdName.isString()) {
    raise_warning("cmdName must be string");
    return null;
  }
  if (!args.isNull() && !args.isArray()) {
    raise_warning("args must be null or array");
    return null;
  }

  // TODO: special handling some command,
  // e.g. multi-line input("<?php"), macro, etc.

  m_client->setCommand(cmdName.toString().data());
  StringVec *clientArgs = m_client->args();
  clientArgs->clear();
  if (!args.isNull()) {
    for (ArrayIter iter(args.toArray()); iter; ++iter) {
      CStrRef arg = iter.second().toString();
      clientArgs->push_back(std::string(arg.data(), arg.size()));
    }
  }
  try {
    if (!m_client->process()) {
      raise_warning("command \"%s\" not found", cmdName.toString().data());
    }
  } catch (DebuggerConsoleExitException &e) {
    // Flow-control command goes here
    Logger::Info("wait for debugger client to stop");
    m_client->setTakingInterrupt();
    m_client->setClientState(DebuggerClient::StateBusy);
    DebuggerCommandPtr cmd = m_client->waitForNextInterrupt();
    if (!cmd || !cmd->is(DebuggerCommand::KindOfInterrupt)) {
      raise_warning("not getting an interrupt");
    } else {
      CmdInterruptPtr cmdInterrupt = dynamic_pointer_cast<CmdInterrupt>(cmd);
      cmdInterrupt->onClient(m_client);
      Logger::Info("debugger client ready for command");
    }
  } catch (DebuggerClientExitException &e) {
    raise_warning("DebuggerClientExitException");
    return null;
  } catch (DebuggerProtocolException &e) {
    raise_warning("DebuggerProtocolException");
    return null;
  }

  return m_client->getPrintString();
}

Variant c_DebuggerClient::t_interrupt() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClient, DebuggerClient::interrupt);
  if (!m_client ||
      m_client->getClientState() < DebuggerClient::StateReadyForCommand) {
    raise_warning("client is not initialized");
    return false;
  }
  if (m_client->getClientState() != DebuggerClient::StateBusy) {
    raise_warning("client is not in a busy state");
    return false;
  }
  m_client->onSignal(SIGINT);
  return m_client->getPrintString();
}

Variant c_DebuggerClient::t___destruct() {
  INSTANCE_METHOD_INJECTION_BUILTIN(DebuggerClient, DebuggerClient::__destruct);
  return null;
}

///////////////////////////////////////////////////////////////////////////////
}
