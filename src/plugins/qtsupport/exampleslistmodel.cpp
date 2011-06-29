#include "exampleslistmodel.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>

#include <QDebug>

#include <coreplugin/icore.h>
#include <qtsupport/qtversionmanager.h>

#include <algorithm>

using QtSupport::QtVersionManager;
using QtSupport::BaseQtVersion;

namespace QtSupport {

namespace Internal {

ExamplesListModel::ExamplesListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roleNames;
    roleNames[Name] = "name";
    roleNames[ProjectPath] = "projectPath";
    roleNames[ImageUrl] = "imageUrl";
    roleNames[Description] = "description";
    roleNames[DocUrl] = "docUrl";
    roleNames[FilesToOpen] = "filesToOpen";
    roleNames[Tags] = "tags";
    roleNames[Difficulty] = "difficulty";
    roleNames[Type] = "type";
    roleNames[HasSourceCode] = "hasSourceCode";
    setRoleNames(roleNames);

    connect(QtVersionManager::instance(), SIGNAL(updateExamples(QString,QString,QString)),
            SLOT(readNewsItems(QString,QString,QString)));
}

QList<ExampleItem> ExamplesListModel::parseExamples(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> examples;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("example")) {
                item = ExampleItem();
                item.type = Example;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                item.imageUrl = attributes.value(QLatin1String("imagePath")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
                m_tags.append(item.tags);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("example"))
                examples.append(item);
            else if (reader->name() == QLatin1String("examples"))
                return examples;
            break;
        default: // nothing
            break;
        }
    }
    return examples;
}

QList<ExampleItem> ExamplesListModel::parseDemos(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> demos;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("demo")) {
                item = ExampleItem();
                item.type = Demo;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("demo"))
                demos.append(item);
            else if (reader->name() == QLatin1String("demos"))
                return demos;
            break;
        default: // nothing
            break;
        }
    }
    return demos;
}

QList<ExampleItem> ExamplesListModel::parseTutorials(QXmlStreamReader* reader, const QString& projectsOffset)
{
    QList<ExampleItem> tutorials;
    ExampleItem item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("tutorial")) {
                item = ExampleItem();
                item.type = Tutorial;
                QXmlStreamAttributes attributes = reader->attributes();
                item.name = attributes.value(QLatin1String("name")).toString();
                item.projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item.hasSourceCode = !item.projectPath.isEmpty();
                item.projectPath.prepend('/');
                item.projectPath.prepend(projectsOffset);
                item.imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                item.docUrl = attributes.value(QLatin1String("docUrl")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item.filesToOpen.append(reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement));
            } else if (reader->name() == QLatin1String("description")) {
                item.description =  reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
            } else if (reader->name() == QLatin1String("tags")) {
                item.tags = reader->readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).split(",");
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("tutorial"))
                tutorials.append(item);
            else if (reader->name() == QLatin1String("tutorials"))
                return tutorials;
            break;
        default: // nothing
            break;
        }
    }

    return tutorials;
}

void ExamplesListModel::readNewsItems(const QString &examplesPath, const QString &demosPath, const QString &sourcePath)
{
    clear();
    foreach (const QString exampleSource, exampleSources()) {
        QFile exampleFile(exampleSource);
        if (!exampleFile.open(QIODevice::ReadOnly)) {
            qDebug() << Q_FUNC_INFO << "Could not open file" << exampleSource;
            return;
        }

        QFileInfo fi(exampleSource);
        QString offsetPath = fi.path();
        QDir examplesDir(offsetPath);
        QDir demosDir(offsetPath);
        if (offsetPath.startsWith(Core::ICore::instance()->resourcePath())) {
            // Try to get dir from first Qt Version
            examplesDir = examplesPath;
            demosDir = demosPath;
        }

        QXmlStreamReader reader(&exampleFile);
        while (!reader.atEnd())
            switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == QLatin1String("examples"))
                    addItems(parseExamples(&reader, examplesDir.path()));
                else if (reader.name() == QLatin1String("demos"))
                    addItems(parseDemos(&reader, demosDir.path()));
                else if (reader.name() == QLatin1String("tutorials"))
                    addItems(parseTutorials(&reader, examplesDir.path()));
                break;
            default: // nothing
                break;
            }

        if (reader.hasError())
            qDebug() << "error parsing file" <<  exampleSource << "as XML document";
    }

    m_tags.sort();
    m_tags.erase(std::unique(m_tags.begin(), m_tags.end()), m_tags.end());
    emit tagsUpdated();
}

QStringList ExamplesListModel::exampleSources() const
{
    QFileInfoList sources;
    const QStringList pattern(QLatin1String("*.xml"));

    // TODO: Read key from settings

    if (sources.isEmpty()) {
        // Try to get dir from first Qt Version
        QtVersionManager *versionManager = QtVersionManager::instance();
        foreach (BaseQtVersion *version, versionManager->validVersions()) {

            QDir examplesDir(version->examplesPath());
            if (examplesDir.exists())
                sources << examplesDir.entryInfoList(pattern);

            QDir demosDir(version->demosPath());
            if (demosDir.exists())
                sources << demosDir.entryInfoList(pattern);

            if (!sources.isEmpty())
                break;
        }
    }

    QString resourceDir = Core::ICore::instance()->resourcePath() + QLatin1String("/welcomescreen/");

    // Try Creator-provided XML file only
    if (sources.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "falling through to Creator-provided XML file";
        sources << QString(resourceDir + QLatin1String("/examples_fallback.xml"));
    }

    sources <<  QString(resourceDir + QLatin1String("/qtcreator_tutorials.xml"));

    QStringList ret;
    foreach (const QFileInfo& source, sources)
        ret.append(source.filePath());

    return ret;
}

void ExamplesListModel::clear()
{
    if (exampleItems.count() > 0) {
        beginRemoveRows(QModelIndex(), 0,  exampleItems.size()-1);
        exampleItems.clear();
        endRemoveRows();
    }
    m_tags.clear();
}

void ExamplesListModel::addItems(const QList<ExampleItem> &newItems)
{
    beginInsertRows(QModelIndex(), exampleItems.size(), exampleItems.size() - 1 + newItems.size());
    exampleItems.append(newItems);
    endInsertRows();
}

int ExamplesListModel::rowCount(const QModelIndex &) const
{
    return exampleItems.size();
}

QVariant ExamplesListModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid() || index.row()+1 > exampleItems.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    ExampleItem item = exampleItems.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole: // for search only
        return QString(item.name + ' ' + item.tags.join(" "));
    case Name:
        return item.name;
    case ProjectPath:
        return item.projectPath;
    case Description:
        return item.description;
    case ImageUrl:
        return item.imageUrl;
    case DocUrl:
        return item.docUrl;
    case FilesToOpen:
        return item.filesToOpen;
    case Tags:
        return item.tags;
    case Difficulty:
        return item.difficulty;
    case HasSourceCode:
        return item.hasSourceCode;
    case Type:
        return item.type;
    default:
        qDebug() << Q_FUNC_INFO << "role type not supported";
        return QVariant();
    }

}

ExamplesListModelFilter::ExamplesListModelFilter(QObject *parent) :
    QSortFilterProxyModel(parent), m_showTutorialsOnly(true)
{
    connect(this, SIGNAL(showTutorialsOnlyChanged()), SLOT(updateFilter()));
}

void ExamplesListModelFilter::updateFilter()
{
    invalidateFilter();
}

bool ExamplesListModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_showTutorialsOnly) {
        int type = sourceModel()->index(sourceRow, 0, sourceParent).data(Type).toInt();
        if (type != Tutorial)
            return false;
        // tag search only active if we are not in tutorial only mode
    } else if (!m_filterTag.isEmpty()) {
        QStringList tags = sourceModel()->index(sourceRow, 0, sourceParent).data(Tags).toStringList();
        if (!tags.contains(m_filterTag, Qt::CaseInsensitive))
            return false;
    }

    bool ok = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    if (!ok)
        return false;

    return true;
}

void ExamplesListModelFilter::setShowTutorialsOnly(bool showTutorialsOnly)
{
    m_showTutorialsOnly = showTutorialsOnly;
    emit showTutorialsOnlyChanged();
}

} // namespace Internal
} // namespace QtSupport