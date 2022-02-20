/*
 *      Copyright (C) 2014-2016 Jean-Luc Barriere
 *
 *  This file is part of Noson
 *
 *  Noson is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Noson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONTENTDIRECTORY_H
#define CONTENTDIRECTORY_H

#include "local_config.h"
#include "service.h"
#include "digitalitem.h"
#include "eventhandler.h"
#include "subscriptionpool.h"
#include "locked.h"

#include <list>
#include <vector>
#include <stdint.h>

#define BROWSE_COUNT  100

namespace NSROOT
{
  class Subscription;

  class ContentDirectory : public Service, public EventSubscriber
  {
  public:
    ContentDirectory(const std::string& serviceHost, unsigned servicePort);
    ContentDirectory(const std::string& serviceHost, unsigned servicePort, SubscriptionPoolPtr& subscriptionPool, void* CBHandle = nullptr, EventCB eventCB = nullptr);
    ~ContentDirectory() override;

    static const std::string Name;
    static const std::string ControlURL;
    static const std::string EventURL;
    static const std::string SCPDURL;

    const std::string& GetName() const override { return Name; }

    const std::string& GetControlURL() const override { return ControlURL; }

    const std::string& GetEventURL() const override { return EventURL; }

    const std::string& GetSCPDURL() const override { return SCPDURL; }

    bool Browse(const std::string& objectId, unsigned index, unsigned count, ElementList& vars);

    bool RefreshShareIndex();

    bool DestroyObject(const std::string& objectID);

    bool CreateObject(const std::string& containerID, const DigitalItemPtr& element);

    // Implements EventSubscriber
    void HandleEventMessage(EventMessagePtr msg) override;

    Locked<ContentProperty>& GetContentProperty() { return m_property; }

  private:
    SubscriptionPoolPtr m_subscriptionPool;
    Subscription m_subscription;
    void* m_CBHandle;
    EventCB m_eventCB;

    Locked<ContentProperty> m_property;
  };

  /////////////////////////////////////////////////////////////////////////////
  ////
  //// ContentSearch
  ////

  typedef enum
  {
    SearchArtist          = 0,
    SearchAlbum,
    SearchGenre,
    SearchTrack,
    SearchComposer,
    SearchContributor,
    SearchPlaylist,
    SearchRadio,
    SearchQueue,
    SearchSonosPlaylist,
    SearchShare,
    SearchFavorite,
    SearchCategory,
    Search_unknown,
  } Search_t;

  class ContentSearch
  {
  public:
    ContentSearch(Search_t search, const std::string& string) : m_search(search), m_string(string) {}
    virtual ~ContentSearch() {}

    virtual std::string Root() const;

    static std::pair<const std::string, const std::string> rootenum(Search_t search);

  protected:
    Search_t m_search;
    std::string m_string;
  };

  /////////////////////////////////////////////////////////////////////////////
  ////
  //// ContentChunk : base class for content
  ////

  class ContentChunk
  {
  public:
    ContentChunk()
    : m_baseUpdateID(0)
    , m_totalCount(0)
    , m_lastUpdateID(0) {}
    virtual ~ContentChunk() {}

    unsigned m_baseUpdateID;
    unsigned m_totalCount;
    unsigned m_lastUpdateID;

    // extract and store summary then return the count of items in the chunk
    unsigned summarize(const ElementList& vars);
  };

  /////////////////////////////////////////////////////////////////////////////
  ////
  //// ContentList
  ////

  class ContentList : private ContentChunk
  {
    typedef std::list<DigitalItemPtr> List;

    friend class iterator;
  public:
    ContentList(ContentDirectory& service, const ContentSearch& search, unsigned bulksize = BROWSE_COUNT);
    ContentList(ContentDirectory& service, const std::string& objectID, unsigned bulksize = BROWSE_COUNT);

    class iterator
    {
      friend class ContentList;
    public:
      typedef iterator self_type;
      iterator() : c(nullptr) {}
      virtual ~iterator() {}
      self_type operator++() { self_type i0 = *this; if (c) c->Next(i); return i0; }
      self_type& operator++(int junk) { (void)junk; if (c) c->Next(i); return *this; }
      self_type operator--() { self_type i0 = *this; if (c) c->Previous(i); return i0; }
      self_type& operator--(int junk) { (void)junk; if (c) c->Previous(i); return *this; }
      bool operator==(const self_type& rhs) const { return rhs.i == i; }
      bool operator!=(const self_type& rhs) const { return rhs.i != i; }
      List::value_type& operator*() const { return *i; }
      List::value_type* operator->() const { return &*i; }
    private:
      ContentList* c;
      List::iterator i;
      iterator(ContentList* _c, const List::iterator& _i) : c(_c) { if (_c) i = _i; }
    };

    bool failure() const { return !m_succeeded; }

    iterator begin() { return iterator(this, m_list.begin()); }

    iterator end() { return iterator(this, m_list.end()); }

    unsigned size() const { return m_totalCount; }

    unsigned GetUpdateID() const { return m_baseUpdateID; }

  private:
    bool m_succeeded;
    ContentDirectory& m_service;
    unsigned m_bulkSize;
    std::string m_root;
    unsigned m_browsedCount;

    List m_list;

    bool Next(List::iterator& i);
    bool Previous(List::iterator& i);
    bool BrowseContent(unsigned startingIndex, unsigned count, List::iterator position);
  };

  /////////////////////////////////////////////////////////////////////////////
  ////
  //// ContentBrowser
  ////

  class ContentBrowser : private ContentChunk
  {
  public:
    typedef std::vector<DigitalItemPtr> Table;

    ContentBrowser(ContentDirectory& service, const ContentSearch& search, unsigned count = BROWSE_COUNT);
    ContentBrowser(ContentDirectory& service, const std::string& objectID, unsigned count = BROWSE_COUNT);

    bool Browse(unsigned startingIndex, unsigned count);

    unsigned index() const { return m_startingIndex; }

    unsigned count() const { return (unsigned) m_table.size(); }

    unsigned total() const { return m_totalCount; }

    Table& table() { return m_table; }

    unsigned GetUpdateID() const { return m_baseUpdateID; }

  private:
    ContentDirectory& m_service;
    std::string m_root;
    unsigned m_startingIndex;

    Table m_table;

    bool BrowseContent(unsigned startingIndex, unsigned count, Table::iterator position);
  };

}

#endif	/* CONTENTDIRECTORY_H */
