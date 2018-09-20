#include "ByteArrayListModel.h"

#include <assert.h>

ByteArrayListModel::ByteArrayListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QString ByteArrayListModel::getFilename() const
{
    return file.fileName();
}

void ByteArrayListModel::saveCacheToFile()
{
    if(!editingCache.empty()){
        for(QMap<qint64, QByteArray>::iterator it = editingCache.begin(); it != editingCache.end(); ++it) {
            const QByteArray &array = *it;
            const qint64 row = it.key();

            file.seek(row * 16);
            qint64 count = file.write(array);

            assert(count == array.size());
        }

        editingCache.clear();
        emit cacheSaved();
    }
}

bool ByteArrayListModel::isEdited() const
{
    return !editingCache.empty();
}

int ByteArrayListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    long ret = 0;
    if(file.isOpen()){
        long size = file.size();
        ret = size / 16 +
                (((size % 16) > 0) ? 1 : 0);
    }
    return ret;
}

QVariant ByteArrayListModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    if(file.isOpen()){
        int row = index.row();

        if(row < rowCount()){
            switch (role) {
            case Qt::DisplayRole:
            {
                QMap<qint64, QByteArray>::const_iterator it = editingCache.find(row);
                bool foundInCache = it != editingCache.end();

                if(foundInCache){
                    ret = *it;
                }else{
                    file.seek(row * 16);
                    ret = file.read(16);
                }
            }
                break;
            default:
                break;
            }
        }
    }

    return ret;
}

QVariant ByteArrayListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant ret;
    if(orientation == Qt::Vertical){
        switch (role) {
        case Qt::DisplayRole:
        {
            qint64 begin = section * 16;
            qint64 end = section + 16 - 1;

            ret = QString::number(begin, 16).rightJustified(4, '0') + " : " +
                    QString::number(end, 16).rightJustified(4, '0');
        }
            break;
        default:
            break;
        }
    }
    return ret;
}

bool ByteArrayListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool ret = false;

    if(file.isOpen()){
        int row = index.row();

        if(row < rowCount()){
            switch (role) {
            case Qt::EditRole:
            {
                assert(value.type() == QVariant::ByteArray);

                const QByteArray originalArray = data(index).toByteArray();
                const QByteArray array = value.toByteArray();

                assert(array.size() <= 16);
                assert(array.size() == originalArray.size());

                editingCache.insert(row, array);

                if(originalArray != array){
                    emit cacheChanged();
                }
            }
                break;
            default:
                break;
            }
        }
    }

    return ret;
}

bool ByteArrayListModel::open(const QString filename)
{
    file.setFileName(filename);

    return file.open(QFile::ReadWrite);
}

Qt::ItemFlags ByteArrayListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}
