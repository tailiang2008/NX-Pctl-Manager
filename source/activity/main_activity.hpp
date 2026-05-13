// MainActivity — root view of the app. Layout lives in resources/xml/activity/main.xml;
// the CONTENT_FROM_XML_RES macro wires the XML into this Activity at creation time.
// Copyright (C) 2026 Taylor.  GPLv3-or-later (see LICENSE).
#pragma once

#include <borealis.hpp>

class MainActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/main.xml");
};
