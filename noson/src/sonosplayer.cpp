/*
 *      Copyright (C) 2014-2015 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "sonosplayer.h"
#include "avtransport.h"
#include "deviceproperties.h"
#include "renderingcontrol.h"
#include "contentdirectory.h"
#include "private/builtin.h"
#include "private/cppdef.h"
#include "private/debug.h"
#include "private/uriparser.h"

using namespace NSROOT;

Player::Player(const std::string& uuid, const std::string& host, unsigned port, EventHandler& eventHandler, void* CBHandle, EventCB eventCB)
: m_uuid(uuid)
, m_host(host)
, m_port(port)
, m_eventHandler(eventHandler)
, m_CBHandle(CBHandle)
, m_eventCB(eventCB)
, m_eventSignaled(false)
, m_eventMask(0)
{
  unsigned subId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(subId, EVENT_HANDLER_STATUS);

  m_AVTSubscription = Subscription(host, port, AVTransport::EventURL, eventHandler.GetPort(), SUBSCRIPTION_TIMEOUT);
  m_RCSSubscription = Subscription(host, port, RenderingControl::EventURL, eventHandler.GetPort(), SUBSCRIPTION_TIMEOUT);
  m_CDSubscription = Subscription(host, port, ContentDirectory::EventURL, eventHandler.GetPort(), SUBSCRIPTION_TIMEOUT);

  m_AVTransport = new AVTransport(m_host, m_port, m_eventHandler, m_AVTSubscription, this, CB_AVTransport);
  m_renderingControl = new RenderingControl(m_host, m_port, m_eventHandler, m_RCSSubscription, this, CB_RenderingControl);
  m_contentDirectory = new ContentDirectory(m_host, m_port, m_eventHandler, m_CDSubscription, this, CB_ContentDirectory);
  m_deviceProperties = new DeviceProperties(m_host, m_port);

  m_AVTSubscription.Start();
  m_RCSSubscription.Start();
  m_CDSubscription.Start();

  m_queueURI.assign("x-rincon-queue:").append(m_uuid).append("#0");
}

Player::~Player()
{
  m_eventHandler.RevokeAllSubscriptions(this);
  SAFE_DELETE(m_contentDirectory);
  SAFE_DELETE(m_deviceProperties);
  SAFE_DELETE(m_renderingControl);
  SAFE_DELETE(m_AVTransport);
}

void Player::RenewSubscriptions()
{
  m_AVTSubscription.AskRenewal();
  m_RCSSubscription.AskRenewal();
  m_CDSubscription.AskRenewal();
}

unsigned char Player::LastEvents()
{
  unsigned char mask;
  Locked<bool>::pointer _signaled = m_eventSignaled.Get();
  {
    Locked<unsigned char>::pointer _mask = m_eventMask.Get();
    mask = *_mask;
    *_mask = 0;
  }
  *_signaled = false;
  return mask;
}

AVTProperty Player::GetTransportProperty()
{
  return *(m_AVTransport->GetAVTProperty().Get());
}

RCSProperty Player::GetRenderingProperty()
{
  return *(m_renderingControl->GetRenderingProperty().Get());
}

ContentProperty Player::GetContentProperty()
{
  return *(m_contentDirectory->GetContentProperty().Get());
}

bool Player::GetZoneInfo(ElementList& vars)
{
  return m_deviceProperties->GetZoneInfo(vars);
}

bool Player::GetTransportInfo(ElementList& vars)
{
  return m_AVTransport->GetTransportInfo(vars);
}

bool Player::GetPositionInfo(ElementList& vars)
{
  return m_AVTransport->GetPositionInfo(vars);
}

bool Player::GetMediaInfo(ElementList& vars)
{
  return m_AVTransport->GetMediaInfo(vars);
}

bool Player::GetVolume(uint8_t* value)
{
  return m_renderingControl->GetVolume(value);
}

bool Player::SetVolume(uint8_t value)
{
  return m_renderingControl->SetVolume(value);
}

bool Player::GetMute(uint8_t* value)
{
  return m_renderingControl->GetMute(value);
}

bool Player::SetMute(uint8_t value)
{
  return m_renderingControl->SetMute(value);
}

bool Player::SetCurrentURI(const DigitalItemPtr& item)
{
  // Check for radio service tuneIN
  if (item->GetObjectID().compare(0, 4, "R:0/") == 0)
  {
    ElementPtr var(new Element("desc", NetServiceDescTable[NetService_TuneIN]));
    var->SetAttribut("id", "cdudn");
    var->SetAttribut("nameSpace", "urn:schemas-rinconnetworks-com:metadata-1-0/");
    item->SetProperty(var);
  }
  return m_AVTransport->SetCurrentURI(item->GetValue("res"), item->DIDL());
}

bool Player::SetCurrentURI(const std::string& uri, const std::string& title)
{
  URIParser _uri(uri);
  if (_uri.Scheme())
  {
    std::string protocolInfo;
    if (strncmp(_uri.Scheme(), "http", 4) == 0)
      protocolInfo.assign(ProtocolTable[Protocol_httpGet]);
    else
      protocolInfo.assign(_uri.Scheme());
    protocolInfo.append(":*:*:*");
    // Setup the digital item
    DigitalItemPtr item(new DigitalItem(DigitalItem::Type_item));
    item->SetProperty(ElementPtr(new Element("dc:title", title)));
    ElementPtr res(new Element("res", uri));
    res->SetAttribut("protocolInfo", protocolInfo);
    item->SetProperty(res);
    return SetCurrentURI(item);
  }
  return false;
}

bool Player::PlayQueue(bool start)
{
  if (m_AVTransport->SetCurrentURI(m_queueURI, ""))
  {
    if (start)
      return m_AVTransport->Play();
    return true;
  }
  return false;
}

unsigned Player::AddURIToQueue(const DigitalItemPtr& item, unsigned position)
{
  return m_AVTransport->AddURIToQueue(item->GetValue("res"), item->DIDL(), position);
}

bool Player::RemoveAllTracksFromQueue()
{
  return m_AVTransport->RemoveAllTracksFromQueue();
}

bool Player::SetPlayMode(PlayMode_t mode)
{
  return m_AVTransport->SetPlayMode(mode);
}

bool Player::Play()
{
  return m_AVTransport->Play();
}

bool Player::Stop()
{
  return m_AVTransport->Stop();
}

bool Player::Pause()
{
  return m_AVTransport->Pause();
}

bool Player::SeekTime(uint16_t reltime)
{
  return m_AVTransport->SeekTime(reltime);
}

bool Player::SeekTrack(unsigned tracknr)
{
  return m_AVTransport->SeekTrack(tracknr);
}

bool Player::Next()
{
  return m_AVTransport->Next();
}

bool Player::Previous()
{
  return m_AVTransport->Previous();
}

ContentDirectory* Player::ContentDirectoryProvider(void* CBHandle, EventCB eventCB)
{
  return new ContentDirectory(GetHost(), GetPort(), m_eventHandler, m_CDSubscription, CBHandle, eventCB);
}

void Player::HandleEventMessage(EventMessagePtr msg)
{
  if (!msg)
    return;
  if (msg->event == EVENT_HANDLER_STATUS)
  {
    // @TODO: handle status
    DBG(DBG_DEBUG, "%s: %s\n", __FUNCTION__, msg->subject[0].c_str());
  }
}

void Player::CB_AVTransport(void* handle)
{
  Player* _handle = static_cast<Player*>(handle);
  Locked<unsigned char>::pointer _mask = _handle->m_eventMask.Get();
  *_mask |= SVCEvent_TransportChanged;
  if (_handle->m_eventCB && !_handle->m_eventSignaled.Load())
    _handle->m_eventCB(_handle->m_CBHandle);
}

void Player::CB_RenderingControl(void* handle)
{
  Player* _handle = static_cast<Player*>(handle);
  Locked<unsigned char>::pointer _mask = _handle->m_eventMask.Get();
  *_mask |= SVCEvent_RenderingControlChanged;
  if (_handle->m_eventCB && !_handle->m_eventSignaled.Load())
    _handle->m_eventCB(_handle->m_CBHandle);
}

void Player::CB_ContentDirectory(void* handle)
{
  Player* _handle = static_cast<Player*>(handle);
  Locked<unsigned char>::pointer _mask = _handle->m_eventMask.Get();
  *_mask |= SVCEvent_ContentDirectoryChanged;
  if (_handle->m_eventCB && !_handle->m_eventSignaled.Load())
    _handle->m_eventCB(_handle->m_CBHandle);
}
