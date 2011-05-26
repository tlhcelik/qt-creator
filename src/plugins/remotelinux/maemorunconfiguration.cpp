/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemorunconfiguration.h"

#include "abstractlinuxdevicedeploystep.h"
#include "maemodeployables.h"
#include "maemoglobal.h"
#include "maemoqemumanager.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"
#include "maemoqtversion.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <qtsupport/qtoutputformatter.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtCore/QStringBuilder>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

namespace {
const bool DefaultUseRemoteGdbValue = false;
} // anonymous namespace

MaemoRunConfiguration::MaemoRunConfiguration(Qt4BaseTarget *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(MAEMO_RC_ID))
    , m_proFilePath(proFilePath)
    , m_useRemoteGdb(DefaultUseRemoteGdbValue)
    , m_baseEnvironmentBase(SystemEnvironmentBase)
    , m_validParse(parent->qt4Project()->validParse(m_proFilePath))
{
    init();
}

MaemoRunConfiguration::MaemoRunConfiguration(Qt4BaseTarget *parent,
        MaemoRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_proFilePath(source->m_proFilePath)
    , m_gdbPath(source->m_gdbPath)
    , m_arguments(source->m_arguments)
    , m_useRemoteGdb(source->useRemoteGdb())
    , m_baseEnvironmentBase(source->m_baseEnvironmentBase)
    , m_systemEnvironment(source->m_systemEnvironment)
    , m_userEnvironmentChanges(source->m_userEnvironmentChanges)
    , m_validParse(source->m_validParse)
{
    init();
}

void MaemoRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
    setUseCppDebugger(true);
    setUseQmlDebugger(false);
    m_remoteMounts = new MaemoRemoteMountsModel(this);

    connect(target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        this, SLOT(handleDeployConfigChanged()));
    handleDeployConfigChanged();

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool)));
    connect(pro, SIGNAL(proFileInvalidated(Qt4ProjectManager::Qt4ProFileNode *)),
            this, SLOT(proFileInvalidated(Qt4ProjectManager::Qt4ProFileNode*)));
}

MaemoRunConfiguration::~MaemoRunConfiguration()
{
}

Qt4BaseTarget *MaemoRunConfiguration::qt4Target() const
{
    return static_cast<Qt4BaseTarget *>(target());
}

Qt4BuildConfiguration *MaemoRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
}

bool MaemoRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration * /* config */) const
{
    if (!m_validParse)
        return false;
    return true;
}

QWidget *MaemoRunConfiguration::createConfigurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

Utils::OutputFormatter *MaemoRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(qt4Target()->qt4Project());
}

void MaemoRunConfiguration::handleParseState(bool success)
{
    bool enabled = isEnabled();
    m_validParse = success;
    if (enabled != isEnabled()) {
        emit isEnabledChanged(!enabled);
    }
}

void MaemoRunConfiguration::proFileInvalidated(Qt4ProjectManager::Qt4ProFileNode *pro)
{
    if (m_proFilePath != pro->path())
        return;
    handleParseState(false);
}

void MaemoRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success)
{
    if (m_proFilePath == pro->path()) {
        handleParseState(success);
        emit targetInformationChanged();
    }
}

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(ArgumentsKey, m_arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_proFilePath));
    map.insert(UseRemoteGdbKey, useRemoteGdb());
    map.insert(BaseEnvironmentBaseKey, m_baseEnvironmentBase);
    map.insert(UserEnvironmentChangesKey,
        Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.unite(m_remoteMounts->toMap());
    return map;
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_arguments = map.value(ArgumentsKey).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_proFilePath = dir.filePath(map.value(ProFileKey).toString());
    m_useRemoteGdb = map.value(UseRemoteGdbKey, DefaultUseRemoteGdbValue).toBool();
    m_userEnvironmentChanges =
        Utils::EnvironmentItem::fromStringList(map.value(UserEnvironmentChangesKey)
        .toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase> (map.value(BaseEnvironmentBaseKey,
        SystemEnvironmentBase).toInt());
    m_remoteMounts->fromMap(map);

    m_validParse = qt4Target()->qt4Project()->validParse(m_proFilePath);

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString MaemoRunConfiguration::defaultDisplayName()
{
    if (!m_proFilePath.isEmpty())
        return (QFileInfo(m_proFilePath).completeBaseName()) + QLatin1String(" (remote)");
    //: Maemo run configuration default display name
    return tr("Run on remote device");
}

MaemoDeviceConfig::ConstPtr MaemoRunConfiguration::deviceConfig() const
{
    const AbstractLinuxDeviceDeployStep * const step = deployStep();
    return step ? step->helper().deviceConfig() : MaemoDeviceConfig::ConstPtr();
}

const QString MaemoRunConfiguration::gdbCmd() const
{
    return QDir::toNativeSeparators(activeBuildConfiguration()->toolChain()->debuggerCommand());
}

Qt4MaemoDeployConfiguration *MaemoRunConfiguration::deployConfig() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(target()->activeDeployConfiguration());
}

AbstractLinuxDeviceDeployStep *MaemoRunConfiguration::deployStep() const
{
    return MaemoGlobal::earlierBuildStep<AbstractLinuxDeviceDeployStep>(deployConfig(), 0);
}

const QString MaemoRunConfiguration::targetRoot() const
{
    QTC_ASSERT(activeQt4BuildConfiguration(), return QString());
    QtSupport::BaseQtVersion *v = activeQt4BuildConfiguration()->qtVersion();
    if (!v)
        return QString();
    return MaemoGlobal::targetRoot(v->qmakeCommand());
}

const QString MaemoRunConfiguration::arguments() const
{
    return m_arguments;
}

QString MaemoRunConfiguration::localDirToMountForRemoteGdb() const
{
    const QString projectDir
        = QDir::fromNativeSeparators(QDir::cleanPath(activeBuildConfiguration()
            ->target()->project()->projectDirectory()));
    const QString execDir
        = QDir::fromNativeSeparators(QFileInfo(localExecutableFilePath()).path());
    const int length = qMin(projectDir.length(), execDir.length());
    int lastSeparatorPos = 0;
    for (int i = 0; i < length; ++i) {
        if (projectDir.at(i) != execDir.at(i))
            return projectDir.left(lastSeparatorPos);
        if (projectDir.at(i) == QLatin1Char('/'))
            lastSeparatorPos = i;
    }
    return projectDir.length() == execDir.length()
        ? projectDir : projectDir.left(lastSeparatorPos);
}

QString MaemoRunConfiguration::remoteProjectSourcesMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfig()->sshParameters().userName)
        + QLatin1String("/gdbSourcesDir_")
        + QFileInfo(localExecutableFilePath()).fileName();
}

QString MaemoRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()
        ->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

QString MaemoRunConfiguration::remoteExecutableFilePath() const
{
    return deployConfig()->deployables()->remoteExecutableFilePath(localExecutableFilePath());
}

MaemoPortList MaemoRunConfiguration::freePorts() const
{
    const Qt4BuildConfiguration * const bc = activeQt4BuildConfiguration();
    const AbstractLinuxDeviceDeployStep * const step = deployStep();
    return bc && step
        ? MaemoGlobal::freePorts(deployStep()->helper().deviceConfig(), bc->qtVersion())
        : MaemoPortList();
}

bool MaemoRunConfiguration::useRemoteGdb() const
{
    if (!m_useRemoteGdb)
        return false;
    const AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(target());
    return maemoTarget && maemoTarget->allowsRemoteMounts();
}

void MaemoRunConfiguration::setArguments(const QString &args)
{
    m_arguments = args;
}

MaemoRunConfiguration::DebuggingType MaemoRunConfiguration::debuggingType() const
{
    const AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(target());
    if (!maemoTarget || !maemoTarget->allowsQmlDebugging())
        return DebugCppOnly;
    if (useCppDebugger()) {
        if (useQmlDebugger())
            return DebugCppAndQml;
        return DebugCppOnly;
    }
    return DebugQmlOnly;
}

int MaemoRunConfiguration::portsUsedByDebuggers() const
{
    switch (debuggingType()) {
    case DebugCppOnly:
    case DebugQmlOnly:
        return 1;
    case DebugCppAndQml:
    default:
        return 2;
    }
}

void MaemoRunConfiguration::updateDeviceConfigurations()
{
    emit deviceConfigurationChanged(target());
}

void MaemoRunConfiguration::handleDeployConfigChanged()
{
    DeployConfiguration * const activeDeployConf
        = target()->activeDeployConfiguration();
    if (activeDeployConf) {
        connect(activeDeployConf->stepList(), SIGNAL(stepInserted(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepInserted(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepMoved(int,int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepRemoved(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        AbstractLinuxDeviceDeployStep * const step
            = MaemoGlobal::earlierBuildStep<AbstractLinuxDeviceDeployStep>(activeDeployConf, 0);
        if (step) {
            connect(&step->helper(), SIGNAL(deviceConfigChanged()),
                SLOT(updateDeviceConfigurations()), Qt::UniqueConnection);
        }
    }

    updateDeviceConfigurations();
    updateFactoryState();
}

QString MaemoRunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == CleanEnvironmentBase)
        return tr("Clean Environment");
    else  if (m_baseEnvironmentBase == SystemEnvironmentBase)
        return tr("System Environment");
    return QString();
}

MaemoRunConfiguration::BaseEnvironmentBase MaemoRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

void MaemoRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase != env) {
        m_baseEnvironmentBase = env;
        emit baseEnvironmentChanged();
    }
}

Utils::Environment MaemoRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Utils::Environment MaemoRunConfiguration::baseEnvironment() const
{
    return (m_baseEnvironmentBase == SystemEnvironmentBase ? systemEnvironment()
        : Utils::Environment());
}

QList<Utils::EnvironmentItem> MaemoRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void MaemoRunConfiguration::setUserEnvironmentChanges(
    const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Utils::Environment MaemoRunConfiguration::systemEnvironment() const
{
    return m_systemEnvironment;
}

void MaemoRunConfiguration::setSystemEnvironment(const Utils::Environment &environment)
{
    if (m_systemEnvironment.size() == 0 || m_systemEnvironment != environment) {
        m_systemEnvironment = environment;
        emit systemEnvironmentChanged();
    }
}

QString MaemoRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

} // namespace Internal
} // namespace RemoteLinux