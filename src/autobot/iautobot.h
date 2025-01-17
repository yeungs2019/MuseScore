/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MU_AUTOBOT_IAUTOBOT_H
#define MU_AUTOBOT_IAUTOBOT_H

#include <vector>
#include <QJSValue>

#include "modularity/imoduleexport.h"
#include "retval.h"
#include "io/path.h"
#include "async/channel.h"
#include "autobottypes.h"
#include "itestcasecontext.h"

namespace mu::autobot {
class IAutobot : MODULE_EXPORT_INTERFACE
{
    INTERFACE_ID(IAutobot)
public:
    virtual ~IAutobot() = default;

    enum class Status {
        Undefined = 0,
        Stopped,
        Running,
        Paused
    };

    virtual Status status() const = 0;
    virtual async::Channel<Status> statusChanged() const = 0;

    virtual Ret execScript(const io::path& path) = 0;

    virtual void setStepsInterval(int msec) = 0;
    virtual void runTestCase(const TestCase& testCase) = 0;
    virtual void sleep(int msec) = 0;
    virtual void pause() = 0;
    virtual void unpause() = 0;
    virtual void abort() = 0;

    virtual async::Channel<QString /*name*/, StepStatus> stepStatusChanged() const = 0;

    virtual ITestCaseContextPtr context() const = 0;
};
}

#endif // MU_AUTOBOT_IAUTOBOT_H
