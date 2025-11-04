#ifndef CANCELLATIONTOKEN_H
#define CANCELLATIONTOKEN_H
#include <atomic>  // 用于线程安全的原子变量

// 前向声明，允许CancellationTokenSource引用CancellationToken
class CancellationToken;

/**
 * 取消令牌源类
 * 用于生成取消令牌，并发送取消信号
 */
class CancellationTokenSource {
public:
    // 构造函数：初始化取消状态为false
    CancellationTokenSource() : m_canceled(false) {}

    // 禁用拷贝构造和赋值（避免多线程下的状态混乱）
    CancellationTokenSource(const CancellationTokenSource&) = delete;
    CancellationTokenSource& operator=(const CancellationTokenSource&) = delete;

    // 发送取消信号（线程安全）
    void cancel() {
        m_canceled = true;
    }

    // 生成一个与当前源关联的取消令牌
    CancellationToken token();

private:
    std::atomic<bool> m_canceled;  // 原子变量，线程安全地存储取消状态
    friend class CancellationToken;  // 允许令牌访问私有成员
};

/**
 * 取消令牌类
 * 用于线程检查是否收到取消信号
 */
class CancellationToken {
public:
    // 检查是否已收到取消信号（线程安全）
    bool isCanceled() const {
        return *m_canceled;
    }

    // 禁用拷贝构造和赋值（保持与源的唯一关联）
    CancellationToken(const CancellationToken&) = delete;
    CancellationToken& operator=(const CancellationToken&) = delete;

private:
    // 仅允许CancellationTokenSource创建令牌
    explicit CancellationToken(std::atomic<bool>* canceled)
        : m_canceled(canceled) {}

    std::atomic<bool>* m_canceled;  // 指向源中的取消状态
    friend class CancellationTokenSource;  // 允许源访问私有构造函数
};

// 实现CancellationTokenSource的token()方法
inline CancellationToken CancellationTokenSource::token() {
    return CancellationToken(&m_canceled);
}
#endif // CANCELLATIONTOKEN_H
