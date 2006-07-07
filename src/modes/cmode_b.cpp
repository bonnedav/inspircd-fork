#include <string>
#include <vector>
#include "inspircd_config.h"
#include "configreader.h"
#include "hash_map.h"
#include "inspircd.h"
#include "mode.h"
#include "channels.h"
#include "users.h"
#include "helperfuncs.h"
#include "message.h"
#include "modules.h"
#include "inspstring.h"
#include "hashcomp.h"
#include "modes/cmode_b.h"

extern InspIRCd* ServerInstance;
extern ServerConfig* Config;
extern std::vector<Module*> modules;
extern std::vector<ircd_module*> factory;
extern int MODCOUNT;
extern time_t TIME;

ModeChannelBan::ModeChannelBan() : ModeHandler('b', 1, 1, true, MODETYPE_CHANNEL, false)
{
}

ModeAction ModeChannelBan::OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding)
{
	int status = cstatus(source, channel);
	adding ? parameter = this->AddBan(source, parameter, channel, status) : parameter = this->DelBan(source, parameter, channel, status);
	return MODEACTION_ALLOW;
}

std::string& ModeChannelBan::AddBan(userrec *user,std::string &dest,chanrec *chan,int status)
{
        BanItem b;
        int toomanyexclamation = 0;
        int toomanyat = 0;

        if ((!user) || (!chan))
        {
                log(DEFAULT,"*** BUG *** AddBan was given an invalid parameter");
		dest = "";
                return dest;
        }

        for (std::string::iterator i = dest.begin(); i != dest.end(); i++)
        {
                if ((*i < 32) || (*i > 126))
                {
			dest = "";
                        return dest;
                }
                else if (*i == '!')
                {
                        toomanyexclamation++;
                }
                else if (*i == '@')
                {
                        toomanyat++;
                }
        }

        if (toomanyexclamation != 1 || toomanyat != 1)
	{
                /*
                 * this stops sillyness like n!u!u!u@h, though note that most
                 * ircds don't actually verify banmask validity. --w00t
                 */
		dest = "";
                return dest;
	}

        long maxbans = GetMaxBans(chan->name);
        if ((unsigned)chan->bans.size() > (unsigned)maxbans)
        {
                WriteServ(user->fd,"478 %s %s :Channel ban list for %s is full (maximum entries for this channel is %d)",user->nick, chan->name,chan->name,maxbans);
		dest = "";
                return dest;
        }

        log(DEBUG,"AddBan: %s %s",chan->name,user->nick);

        int MOD_RESULT = 0;
        FOREACH_RESULT(I_OnAddBan,OnAddBan(user,chan,dest));
        if (MOD_RESULT)
	{
		dest = "";
                return dest;
	}

        //TidyBan(dest);
        for (BanList::iterator i = chan->bans.begin(); i != chan->bans.end(); i++)
        {
                if (!strcasecmp(i->data,dest.c_str()))
                {
                        // dont allow a user to set the same ban twice
			dest = "";
                        return dest;
                }
        }

        b.set_time = TIME;
        strlcpy(b.data,dest.c_str(),MAXBUF);
        if (*user->nick)
        {
                strlcpy(b.set_by,user->nick,NICKMAX-1);
        }
        else
        {
                strlcpy(b.set_by,Config->ServerName,NICKMAX-1);
        }
        chan->bans.push_back(b);
        return dest;
}

std::string& ModeChannelBan::DelBan(userrec *user,std::string& dest,chanrec *chan,int status)
{
        if ((!user) || (!chan)) {
                log(DEFAULT,"*** BUG *** TakeBan was given an invalid parameter");
		dest = "";
                return dest;
        }

        log(DEBUG,"del_ban: %s %s",chan->name,user->nick);
        for (BanList::iterator i = chan->bans.begin(); i != chan->bans.end(); i++)
        {
                if (!strcasecmp(i->data,dest.c_str()))
                {
                        int MOD_RESULT = 0;
                        FOREACH_RESULT(I_OnDelBan,OnDelBan(user,chan,dest));
                        if (MOD_RESULT)
			{
				dest = "";
                                return dest;
			}
                        chan->bans.erase(i);
                        return dest;
                }
        }
	dest = "";
        return dest;
}

