#ifndef DOWNLOAD_ITEM_H
#define DOWNLOAD_ITEM_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include <QString>
#include <QUrl>

namespace dm{

namespace model{

struct random{};
struct url{};

struct download_item
{    
    enum class tag{
        name,
        percent,
        size,
        status
    };

    download_item() = default;
    download_item(QString const &status,
                  QUrl const &url);

    QString name_;
    float percent_ = 0;
    size_t size_ = 0;
    QString status_ = "Waiting";
    QUrl url_;
};

using download_index = boost::multi_index::multi_index_container
<
    download_item,
    boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<random>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<url>,
            boost::multi_index::member<download_item,QUrl,&download_item::url_>
        >
    >
>;

}

}

#endif // DOWNLOAD_ITEM_H
