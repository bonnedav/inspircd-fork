/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2008 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "configreader.h"
#include "users.h"
#include "modules.h"
#include "commands/cmd_rehash.h"



extern "C" DllExport command_t* init_command(InspIRCd* Instance)
{
	return new cmd_rehash(Instance);
}

CmdResult cmd_rehash::Handle (const char** parameters, int pcnt, userrec *user)
{
	user->WriteServ("382 %s %s :Rehashing",user->nick,ServerConfig::CleanFilename(ServerInstance->ConfigFileName));
	std::string parameter;
	std::string old_disabled = ServerInstance->Config->DisabledCommands;
	if (pcnt)
	{
		parameter = parameters[0];
	}

	// If it's not got a param or the param doesn't look like a module-owned one (e.g. servermask)
	if (!pcnt || parameters[0][0] != "-")
	{
		ServerInstance->WriteOpers("*** %s is rehashing config file %s",user->nick,ServerConfig::CleanFilename(ServerInstance->ConfigFileName));
		ServerInstance->CloseLog();
		if (!ServerInstance->OpenLog(ServerInstance->Config->argv, ServerInstance->Config->argc))
			user->WriteServ("*** NOTICE %s :ERROR: Could not open logfile %s: %s", user->nick, ServerInstance->Config->logpath.c_str(), strerror(errno));
		ServerInstance->RehashUsersAndChans();
		FOREACH_MOD(I_OnGarbageCollect, OnGarbageCollect());
		ServerInstance->Config->Read(false,user);
		ServerInstance->Res->Rehash();
		ServerInstance->ResetMaxBans();
	}
	if (old_disabled != ServerInstance->Config->DisabledCommands)
		InitializeDisabledCommands(ServerInstance->Config->DisabledCommands, ServerInstance);

	FOREACH_MOD(I_OnRehash,OnRehash(user, parameter));

	ServerInstance->BuildISupport();

	return CMD_SUCCESS;
}
