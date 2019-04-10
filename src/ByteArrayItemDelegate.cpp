#include "ByteArrayItemDelegate.h"

#include <assert.h>

#include <QApplication>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QPainter>

constexpr char ByteArrayItemDelegate::representationSeparator[];

ByteArrayItemDelegate::ByteArrayItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    codec(nullptr)
{
}

QString ByteArrayItemDelegate::getHexRepresentation(const QByteArray &array,
                                                    bool justified) const
{
    QString str = array.toHex(' ');

    if(justified){
        str = str.leftJustified(justifiedLen, ' ');
    }

    return str;
}

QString ByteArrayItemDelegate::getAsciiRepresentation(const QByteArray &array) const
{
    QString str;
    if(codec != nullptr){
        str = codec->toUnicode(array);
    }else{
        str = array;
    }

    for(QChar &c : str){
        const bool printAsIs = c.isLetterOrNumber() ||
                c.isPunct();

        if(!printAsIs){
            c = '.';
        }
    }

    return str;
}

void ByteArrayItemDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if(option.state & QStyle::State_Selected){
        painter->fillRect(option.rect, QBrush(Qt::cyan));
    }else{
        painter->fillRect(option.rect, QBrush(Qt::white));
    }

    painter->setFont(option.font);
    {
        const int siblingCol = (index.column() == 1) ? 0 : 1;
        const QModelIndex siblingIndex = index.sibling(index.row(), siblingCol);
        if(siblingIndex.isValid()){
            const QByteArray thisArray = index.data().toByteArray();
            const QByteArray thatArray = siblingIndex.data().toByteArray();

            int i = 0;
            while((i < thisArray.length())){
                i = drawHighlighting(painter, thisArray, thatArray, i, option.rect);
            }
        }
    }

    QString thisStr = getText(index);
    painter->drawText(option.rect, Qt::AlignVCenter, thisStr);
}

int ByteArrayItemDelegate::drawHighlighting(QPainter *painter, const QByteArray &thisArray,
                                            const QByteArray &thatArray, const int pos,
                                            const QRect &rect) const
{
    int i = pos;
    bool diff = false;

    while((i < thisArray.length())){
        if((thatArray.length() <= i) ||
                (thatArray[i] != thisArray[i])){
            diff = true;
            i++;
        }else{
            break;
        }
    }

    if(diff){
        const QFontMetrics fontMetrics(painter->font());
        painter->save();
        painter->setBrush(QBrush(Qt::yellow));
        painter->setPen(QPen(Qt::NoPen));
        {
            assert(i > 0);
            const int begin = rect.left() +
                    fontMetrics.width(getHexRepresentation(thisArray, true).left(i * 3 - 1));
            const int end = rect.left() +
                    fontMetrics.width(getHexRepresentation(thisArray, true).left(pos * 3));

            const QRect redrawRect(QPoint(begin, rect.top()), QPoint(end, rect.bottom()));
            painter->drawRect(redrawRect);
        }
        {
            const int begin = rect.left() +
                    fontMetrics.width(getText(thisArray).left(i + justifiedLen + separatorLen));
            const int end = rect.left() +
                    fontMetrics.width(getText(thisArray).left(pos + justifiedLen + separatorLen));

            const QRect redrawRect(QPoint(begin, rect.top()), QPoint(end, rect.bottom()));
            painter->drawRect(redrawRect);
        }
        painter->restore();
    }
    return i + 1;
}

QWidget *ByteArrayItemDelegate::createEditor(QWidget *parent,
                                             const QStyleOptionViewItem &/* option */,
                                             const QModelIndex &/* index */) const
{
    QLineEdit *const editor = new QLineEdit(parent);
    editor->setFrame(false);

    const QRegExp regexp("[0-f]{1,2}(\\s[0-f]{1,2})*$");
    editor->setValidator(new QRegExpValidator(regexp, editor));

    return editor;
}

void ByteArrayItemDelegate::setEditorData(QWidget *editor,
                                          const QModelIndex &index) const
{
    const QByteArray array = index.data().toByteArray();

    QLineEdit *const lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(getHexRepresentation(array));
}

void ByteArrayItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                         const QModelIndex &index) const
{
    const QVariant originalValue = model->data(index, Qt::DisplayRole);

    assert(originalValue.type() == QVariant::ByteArray);

    const QByteArray originalArray = originalValue.toByteArray();
    const int originalArraySize = originalArray.size();

    const QLineEdit *const lineEdit = static_cast<QLineEdit*>(editor);

    const QRegExp regexp("[0-f]{2}(\\s[0-f]{2})*$");
    const QString hexString = lineEdit->text();
    if(regexp.exactMatch(hexString)){
        const QByteArray array = QByteArray::fromHex(hexString.toLatin1());

        if(array.size() == originalArraySize){
            model->setData(index, array, Qt::EditRole);
        }
    }
}

void ByteArrayItemDelegate::updateEditorGeometry(QWidget *editor,
                                                 const QStyleOptionViewItem &option,
                                                 const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

QSize ByteArrayItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QString text = getText(index);

    return sizeHint(text, option.font);
}

QSize ByteArrayItemDelegate::sizeHint(const QString &text, const QFont &font) const
{
    const QFontMetrics fontMetrics(font);
    const QSize s(fontMetrics.width(text), fontMetrics.height());

    return s;
}

QString ByteArrayItemDelegate::getText(const QModelIndex &index) const
{
    const QByteArray thisArray = index.data().toByteArray();

    return getText(thisArray);
}

QString ByteArrayItemDelegate::getText(const QByteArray &thisArray) const
{
    return getHexRepresentation(thisArray, true) +
            representationSeparator + getAsciiRepresentation(thisArray);
}

void ByteArrayItemDelegate::setCodec(QTextCodec * codec) const
{
    this->codec = codec;
}

