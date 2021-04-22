/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Zeeno from Atlantis wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Zeeno from Atlantis
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef _ZEENO_SIGNAL_HPP
#	define _ZEENO_SIGNAL_HPP

#	include <algorithm>
#	include <atomic>
#	include <functional>
#	include <memory>
#	include <mutex>
#	include <vector>

namespace Zeeno {

struct dummy_mutex {
	void lock() {
	}
	void unlock() {
	}
	bool try_lock() {
		return true;
	}
};

struct Connection {
	virtual ~Connection()        = default;
	virtual bool isValid() const = 0;
	virtual bool disconnect()    = 0;

	inline operator bool() const {
		return isValid();
	}
};

template<typename Mutex, typename... Arguments>
class SignalWithMutex {
public:
	using Callback = std::function<void(Arguments...)>;

	struct Connection final : Zeeno::Connection {
		friend class SignalWithMutex<Mutex, Arguments...>;

		bool isValid() const override {
			std::lock_guard<Mutex> lock{m_mutex};
			return m_connected;
		}
		bool disconnect() override {
			std::lock_guard<Mutex> lock{m_mutex};
			if (m_connected) {
				bool ret    = m_connected->disconnect(this);
				m_connected = nullptr;
				return ret;
			}
			return false;
		}

	protected:
		void setConnected(SignalWithMutex<Mutex, Arguments...>* sig) {
			std::lock_guard<Mutex> lock{m_mutex};
			m_connected = sig;
		}

	private:
		SignalWithMutex<Mutex, Arguments...>* m_connected{nullptr};
		Callback                              m_callback;
		mutable Mutex                         m_mutex;
	};
	friend struct Connection;

	std::shared_ptr<Zeeno::Connection> connect(Callback f) {
		std::shared_ptr<Connection> c = std::make_shared<Connection>();
		c->m_callback                 = std::move(f);
		c->m_connected                = this;
		{
			std::lock_guard<Mutex> lock{m_mutex};
			m_functions.push_back(c);
			m_dirty = true;
		}
		return std::move(c);
	}

	void operator()(Arguments... args) const {
		if (isBlocked())
			return;

		if (m_dirty) {
			std::lock_guard<Mutex> lock{m_mutex};
			if (m_dirty) {
				std::lock_guard<Mutex> cacheLock{m_cacheMutex};
				m_cache = m_functions;
				m_dirty = false;
			}
		}

		{
			std::lock_guard<Mutex> cacheLock{m_cacheMutex};
			std::for_each(m_cache.cbegin(), m_cache.cend(), [&args...](const std::shared_ptr<Connection>& c) { c->m_callback(args...); });
		}
	}

	inline void notify(Arguments... args) const {
		operator()(args...);
	}

	inline bool disconnect(const std::shared_ptr<Connection>& c) {
		std::lock_guard<Mutex> lock{m_mutex};
		auto                   it = std::find(m_functions.cbegin(), m_functions.cend(), c);
		if (it == m_functions.end()) {
			return false;
		}
		(*it)->setConnected(nullptr);
		m_functions.erase(it);
		m_dirty = true;
		return true;
	}

	inline void disconnectAll() {
		std::lock_guard<Mutex> lock{m_mutex};
		std::for_each(m_functions.begin(), m_functions.end(), [](const std::shared_ptr<Connection>& c) { c.setConnected(nullptr); });
		m_functions.clear();
	}

	inline void blockSignal() {
		m_block = true;
	}

	inline bool isBlocked() const {
		return m_block;
	}

	inline void unblockSignal() {
		m_block = false;
	}

	inline bool isConnected(const Connection& c) const {
		std::lock_guard<Mutex> lock{m_mutex};
		return std::find(m_functions.cbegin(), m_functions.cend(), c) != m_functions.cend();
	}

private:
	inline bool disconnect(const Connection* c) {
		std::lock_guard<Mutex> lock{m_mutex};
		auto                   it = std::find_if(m_functions.cbegin(), m_functions.cend(), [c](const std::shared_ptr<Connection>& s) { return s.get() == c; });
		if (it == m_functions.end()) {
			return false;
		}
		m_functions.erase(it);
		m_dirty = true;
		return true;
	}

private:
	std::vector<std::shared_ptr<Connection>>         m_functions;
	bool                                             m_block{false};
	mutable Mutex                                    m_mutex;

	mutable std::atomic_bool                         m_dirty{false};
	mutable std::vector<std::shared_ptr<Connection>> m_cache;
	mutable Mutex                                    m_cacheMutex;
};

template<typename... Arguments>
class SignalWithMutex<dummy_mutex, Arguments...> {
public:
	using Callback = std::function<void(Arguments...)>;

	struct Connection final : Zeeno::Connection {
		friend class SignalWithMutex<dummy_mutex, Arguments...>;

		bool isValid() const override {
			return m_connected;
		}
		bool disconnect() override {
			return m_connected ? m_connected->disconnect(this) : false;
		}

	protected:
		void setConnected(SignalWithMutex<dummy_mutex, Arguments...>* sig) {
			m_connected = sig;
		}

	private:
		SignalWithMutex<dummy_mutex, Arguments...>* m_connected{nullptr};
		Callback                                    m_callback;
	};

	std::shared_ptr<Zeeno::Connection> connect(Callback f) {
		std::shared_ptr<Connection> c = std::make_shared<Connection>();
		c->m_callback                 = std::move(f);
		c->setConnected(this);
		m_functions.push_back(c);
		return std::move(c);
	}

	void operator()(Arguments... args) const {
		if (isBlocked())
			return;
		std::for_each(m_functions.cbegin(), m_functions.cend(), [&args...](const std::shared_ptr<Connection>& c) { c->m_callback(args...); });
	}

	inline void notify(Arguments... args) const {
		operator()(args...);
	}

	inline bool disconnect(const std::shared_ptr<Connection>& c) {
		auto it = std::find(m_functions.cbegin(), m_functions.cend(), c);
		if (it == m_functions.end())
			return false;
		(*it)->setConnected(nullptr);
		m_functions.erase(it);
		return true;
	}

	inline bool disconnect(const Connection* c) {
		auto it = std::find_if(m_functions.cbegin(), m_functions.cend(), [c](const std::shared_ptr<Connection>& s) { return s.get() == c; });
		if (it == m_functions.end())
			return false;
		(*it)->setConnected(nullptr);
		m_functions.erase(it);
		return true;
	}

	inline void disconnectAll() {
		std::for_each(m_functions.begin(), m_functions.end(), [](const std::shared_ptr<Connection>& c) { c->setConnected(nullptr); });
		m_functions.clear();
	}

	inline void blockSignal() {
		m_block = true;
	}

	inline bool isBlocked() const {
		return m_block;
	}

	inline void unblockSignal() {
		m_block = false;
	}

	inline bool isConnected(const Connection& c) const {
		return std::find(m_functions.cbegin(), m_functions.cend(), c) != m_functions.cend();
	}

private:
	std::vector<std::shared_ptr<Connection>> m_functions;
	bool                                     m_block{false};
};

template<typename... Args>
using Signal = SignalWithMutex<dummy_mutex, Args...>;

} // namespace Zeeno

#endif
