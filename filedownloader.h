#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class FileDownloader : public QObject {
    Q_OBJECT
public:
    explicit FileDownloader(int id, QUrl imageUrl, QObject *parent = 0);
    void Refresh();
    virtual ~FileDownloader();
    QByteArray downloadedData() const;
    QUrl CurrentUrl() const;
    void newUrl(const QUrl &value);
signals:
    void downloaded(int);
private slots:
    void fileDownloaded(QNetworkReply *pReply);
private:
    QNetworkAccessManager m_WebCtrl;
    QByteArray  m_DownloadedData;
    QUrl        myUrl;
    int         myId;
};

#endif // FILEDOWNLOADER_H
