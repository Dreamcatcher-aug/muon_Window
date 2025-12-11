#ifndef CANCELLATIONTOKEN_H
#define CANCELLATIONTOKEN_H
#include <atomic>

class CancellationToken;

class CancellationTokenSource
{
public:
    CancellationTokenSource() : m_canceled(false) {}
    CancellationTokenSource(const CancellationTokenSource&) = delete;
    CancellationTokenSource& operator=(const CancellationTokenSource&) = delete;
    void cancel()
    {
        m_canceled = true;
    }
    CancellationToken token();

private:
    std::atomic<bool> m_canceled;
    friend class CancellationToken;
};

class CancellationToken
{
public:
    bool isCanceled() const
    {
        return *m_canceled;
    }
    CancellationToken(const CancellationToken&) = delete;
    CancellationToken& operator=(const CancellationToken&) = delete;

private:
    explicit CancellationToken(std::atomic<bool>* canceled)
        : m_canceled(canceled) {}
    std::atomic<bool>* m_canceled;
    friend class CancellationTokenSource;
};

inline CancellationToken CancellationTokenSource::token()
{
    return CancellationToken(&m_canceled);
}
#endif // CANCELLATIONTOKEN_H
