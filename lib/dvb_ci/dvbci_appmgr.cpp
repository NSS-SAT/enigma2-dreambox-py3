/* DVB CI Application Manager */

#include <lib/base/eerror.h>
#include <lib/dvb_ci/dvbci_appmgr.h>
#include <lib/dvb_ci/dvbci_ui.h>
#include <lib/base/estring.h>

eDVBCIApplicationManagerSession::eDVBCIApplicationManagerSession(eDVBCISlot *tslot)
{
	slot = tslot;
	slot->setAppManager(this);
	m_app_name = "";
}

eDVBCIApplicationManagerSession::~eDVBCIApplicationManagerSession()
{
	slot->setAppManager(NULL);
}

int eDVBCIApplicationManagerSession::receivedAPDU(const unsigned char *tag,const void *data, int len)
{
	eTraceNoNewLine("[CI AM] SESSION(%d)/APP %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
	for (int i=0; i<len; i++)
		eTraceNoNewLine("%02x ", ((const unsigned char*)data)[i]);
	eTraceNoNewLine("\n");

	if ((tag[0]==0x9f) && (tag[1]==0x80))
	{
		switch (tag[2])
		{
		case 0x21:
		{
			int dl;
			eDebug("[CI AM] application info:");
			eDebug("[CI AM]   len: %d", len);
			eDebug("[CI AM]   application_type: %d", ((unsigned char*)data)[0]);
			eDebug("[CI AM]   application_manufacturer: %02x %02x", ((unsigned char*)data)[2], ((unsigned char*)data)[1]);
			eDebug("[CI AM]   manufacturer_code: %02x %02x", ((unsigned char*)data)[4],((unsigned char*)data)[3]);
			dl=((unsigned char*)data)[5];
			if ((dl + 6) > len)
			{
				eDebug("[CI AM] warning, invalid length (%d vs %d)", dl+6, len);
				dl=len-6;
			}
			char str[dl + 1];
			memcpy(str, ((char*)data) + 6, dl);
			str[dl] = '\0';
			eDebugNoNewLine("[CI AM]   menu string: ");
			for (int i = 0; i < dl; ++i)
				eDebugNoNewLine("%c", ((unsigned char*)data)[i+6]);
			eDebugNoNewLine("\n");

			m_app_name = str;
			if(m_app_name.size() > 0 && !isUTF8(m_app_name)) {
				m_app_name = repairUTF8(m_app_name.c_str(), m_app_name.size());
				eDebug("[CI AM]   fixed menu string: %s", m_app_name.c_str());
			}
			/* emit */ eDVBCI_UI::getInstance()->m_messagepump.send(eDVBCIInterfaces::Message(eDVBCIInterfaces::Message::appNameChanged, slot->getSlotID(), m_app_name.c_str()));

			/* emit */ eDVBCI_UI::getInstance()->m_messagepump.send(eDVBCIInterfaces::Message(eDVBCIInterfaces::Message::slotStateChanged, slot->getSlotID(), 2));
			break;
		}
		default:
			eWarning("[CI AM] unknown APDU tag 9F 80 %02x", tag[2]);
			break;
		}
	}
	return 0;
}

int eDVBCIApplicationManagerSession::doAction()
{
  switch (state)
  {
  case stateStarted:
  {
    const unsigned char tag[3]={0x9F, 0x80, 0x20}; // application manager info e    sendAPDU(tag);
		sendAPDU(tag);
    state=stateFinal;
    return 1;
  }
  case stateFinal:
    eDebug("[CI AM] in final state.");
		wantmenu = 0;
    if (wantmenu)
    {
      eDebug("[CI AM] wantmenu: sending Tenter_menu");
      const unsigned char tag[3]={0x9F, 0x80, 0x22};  // Tenter_menu
      sendAPDU(tag);
      wantmenu=0;
      return 0;
    } else
      return 0;
  default:
    return 0;
  }
}

int eDVBCIApplicationManagerSession::startMMI()
{
	eDebug("[CI AM] in appmanager -> startmmi()");
	const unsigned char tag[3]={0x9F, 0x80, 0x22};  // Tenter_menu
	sendAPDU(tag);
	return 0;
}

