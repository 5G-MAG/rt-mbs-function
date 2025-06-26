#ifndef _MBSF_APP_HH_
#define _MBSF_APP_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Main app entry point
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

#include <memory>
#include <string>

#include "common.hh"
#include "Context.hh"
#include "EventHandler.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"
#include "MBSFNetworkFunction.hh"
#include "NfServer.hh"
#include "Open5GSNetworkFunction.hh"
#include "Open5GSYamlDocument.hh"

extern "C" {
    extern int app_initialize(const char *const argv[]);
    extern void app_terminate(void);
}

MBSF_NAMESPACE_START

class App {
public:
    App(const char *const argv[]);
    App() = delete;
    App(App &&other) = delete;
    App(const App &other) = delete;
    App &operator=(App &&other) = delete;
    App &operator=(const App &other) = delete;
    virtual ~App();

    static const App &self(); // Get singleton self

    void initialise();
    void startEventHandler();

    const std::shared_ptr<Open5GSNetworkFunction> &ogsApp() const { return m_app; };
    const std::shared_ptr<Context> &context() const { return m_context; };

    EventHandler *registerEventHandler(EventHandler *event_handler);
    Open5GSYamlDocument configDocument() const;

    const NfServer::AppMetadata &mbsfAppMetadata() const;
    const std::string &serverName();
    const std::string &serverName() const { return m_appMetadata.serverName(); };
    int queuePush(const std::shared_ptr<Open5GSEvent> &event) const;

protected:
    friend class Open5GSNetworkFunction;
    void dispatchEvent(Open5GSFSM &fsm, Open5GSEvent &event) const;

private:
    std::shared_ptr<Open5GSNetworkFunction> m_app;
    std::shared_ptr<Context>                m_context;
    NfServer::AppMetadata                   m_appMetadata;
    EventHandler                           *m_eventHandler;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_APP_HH_ */
