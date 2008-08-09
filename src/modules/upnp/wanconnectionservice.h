//=============================================================================
//
//   Creation date : Fri Aug 08 18:00:00 2008 GMT by Fabio Bas
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2008 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//   Original Copyright follows:
//=============================================================================

/***************************************************************************
                          wanconnectionservice.h -  description
                             -------------------
    begin                : Mon Jul 25 2005
    copyright            : (C) 2005 by Diederik van der Boor
    email                : vdboor --at-- codingdomain.com
 ***************************************************************************/

#ifndef UPNP_WANCONNECTIONSERVICE_H
#define UPNP_WANCONNECTIONSERVICE_H

#include "service.h"
#include "kvi_pointerlist.h"

namespace UPnP
{

struct PortMapping
{
  QString  protocol;
  QString  remoteHost;
  int      externalPort;
  QString  internalClient;
  int      internalPort;
  int      leaseDuration;
  QString  description;
  bool     enabled;
};


/**
 * The Wan(IP/PPP)Connection service controls the connection and port forwarding settings of a router.
 * The Layer3ForwardingService result either defines a WanIPConnection or WanPPPConnection service.
 * Which one is returned depends on the external connection type.
 * This class implements the common actions both services support,
 * which is sufficient to control the port mappings of the router.
 *
 * @author Diederik van der Boor
 * @ingroup NetworkUPnP
 */
class WanConnectionService : public Service
{
  public:  // public methods

    // The constructor
                         WanConnectionService(const ServiceParameters &params);
    // The destructor
    virtual             ~WanConnectionService();

    // Add a port mapping
    void                 addPortMapping(const QString &protocol, const QString &remoteHost, int externalPort,
                                        const QString &internalClient, int internalPort, const QString &description,
                                        bool enabled = true, int leaseDuration = 0);
    // Delete a port mapping
    void                 deletePortMapping(const QString &protocol, const QString &remoteHost, int externalPort);

    // Return the external IP address
    QString              getExternalIpAddress() const;
    // Return true if NAT is enabled
    bool                 getNatEnabled() const;
    // Return the port mappings
    const KviPointerList<PortMapping>& getPortMappings() const;

    // Query for the external IP address
    void                 queryExternalIpAddress();
    // Query for the Nat status
    void                 queryNatEnabled();
    // Query for a port mapping entry
    void                 queryPortMappingEntry(int index);


  protected:  // protected methods

    // The control point received a response to callAction()
    virtual void         gotActionResponse(const QString &responseType, const QMap<QString,QString> &resultValues);


  private:  // private attributes
    // The external IP address
    QString              externalIpAddress_;
    // True if NAT is enabled
    bool                 natEnabled_;
    // The current port mappings
    KviPointerList<PortMapping> portMappings_;
};

}

#endif
