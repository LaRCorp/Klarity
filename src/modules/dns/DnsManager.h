#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class DnsManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString activeServer READ activeServer NOTIFY dnsChanged)
    Q_PROPERTY(QString activeProvider READ activeProvider NOTIFY dnsChanged)
    Q_PROPERTY(QString activeInterface READ activeInterface NOTIFY dnsChanged)
    Q_PROPERTY(QString activeConnection READ activeConnection NOTIFY dnsChanged)
    Q_PROPERTY(QStringList dnsServers READ dnsServers NOTIFY dnsChanged)
    Q_PROPERTY(QString encryptionStatus READ encryptionStatus NOTIFY dnsChanged)
    Q_PROPERTY(bool isEncrypted READ isEncrypted NOTIFY dnsChanged)

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isBenchmarking READ isBenchmarking NOTIFY isBenchmarkingChanged)
    Q_PROPERTY(bool isCheckingLeak READ isCheckingLeak NOTIFY isCheckingLeakChanged)

    Q_PROPERTY(QVariantList benchmarkResults READ benchmarkResults NOTIFY benchmarkResultsChanged)
    Q_PROPERTY(QVariantMap leakResult READ leakResult NOTIFY leakResultChanged)
    Q_PROPERTY(QVariantMap checkResult READ checkResult NOTIFY checkResultChanged)
    Q_PROPERTY(QVariantList availableProviders READ availableProviders CONSTANT)

public:
    explicit DnsManager(QObject *parent = nullptr);
    ~DnsManager() override = default;

    QString activeServer() const { return m_activeServer; }
    QString activeProvider() const { return m_activeProvider; }
    QString activeInterface() const { return m_activeInterface; }
    QString activeConnection() const { return m_activeConnection; }
    QStringList dnsServers() const { return m_dnsServers; }
    QString encryptionStatus() const { return m_encryptionStatus; }
    bool isEncrypted() const { return m_isEncrypted; }

    bool isLoading() const { return m_isLoading; }
    bool isBenchmarking() const { return m_isBenchmarking; }
    bool isCheckingLeak() const { return m_isCheckingLeak; }

    QVariantList benchmarkResults() const { return m_benchmarkResults; }
    QVariantMap leakResult() const { return m_leakResult; }
    QVariantMap checkResult() const { return m_checkResult; }
    QVariantList availableProviders() const;

    Q_INVOKABLE void refreshCurrentDns();
    Q_INVOKABLE void setDnsProvider(const QString &providerId, const QString &customServers = QString());
    Q_INVOKABLE void checkDns(const QString &domain = QStringLiteral("google.com"));
    Q_INVOKABLE void compareProviders(const QString &domain = QStringLiteral("google.com"));
    Q_INVOKABLE void checkLeaks();
    Q_INVOKABLE void openLeakTestInBrowser();

signals:
    void dnsChanged();
    void isLoadingChanged();
    void isBenchmarkingChanged();
    void isCheckingLeakChanged();
    void benchmarkResultsChanged();
    void leakResultChanged();
    void checkResultChanged();
    void operationFinished(bool success, const QString &message);

private:
    void parseResolvectlStatus(const QString &output);
    void parseNmcliStatus();
    QString detectProvider(const QString &ip, const QStringList &allServers);

    QString m_activeServer;
    QString m_activeProvider;
    QString m_activeInterface;
    QString m_activeConnection;
    QStringList m_dnsServers;
    QString m_encryptionStatus;
    bool m_isEncrypted = false;

    bool m_isLoading = false;
    bool m_isBenchmarking = false;
    bool m_isCheckingLeak = false;

    QVariantList m_benchmarkResults;
    QVariantMap m_leakResult;
    QVariantMap m_checkResult;
};
