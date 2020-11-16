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
#define _ZEENO_SIGNAL_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace Zeeno {

struct Connection {
	virtual ~Connection()        = default;
	virtual bool isValid() const = 0;
	virtual bool disconnect()    = 0;
	inline       operator bool() const {
        return isValid();
	}
};

template <typename... Arguments> class Signal {
public:
	using Callback = std::function<void(Arguments...)>;

	struct Connection final : Zeeno::Connection {
		friend class Signal<Arguments...>;

		bool isValid() const override {
			return m_connected;
		}
		bool disconnect() override {
			return m_connected ? m_connected->disconnect(this) : false;
		}

	private:
		Signal<Arguments...>* m_connected{nullptr};
		Callback              m_callback;
	};

	std::shared_ptr<Zeeno::Connection> connect(Callback f) {
		std::shared_ptr<Connection> c = std::make_shared<Connection>();
		c->m_callback                 = std::move(f);
		c->m_connected                = this;
		m_functions.push_back(c);
		return std::move(c);
	}

	void operator()(Arguments... args) const {
		if (!isBlocked())
			std::for_each(m_functions.begin(), m_functions.end(), [&args...](const std::shared_ptr<Connection>& c) {
				c->m_callback(args...);
			});
	}

	inline void notify(Arguments... args) const {
		operator()(args...);
	}

	inline bool disconnect(const std::shared_ptr<Connection>& c) {
		auto it = std::find(m_functions.cbegin(), m_functions.cend(), c);
		if (it == m_functions.end())
			return false;
		(*it)->m_connected = nullptr;
		m_functions.erase(it);
		return true;
	}

	inline bool disconnect(const Connection* c) {
		auto it = std::find_if(m_functions.cbegin(), m_functions.cend(), [c](const std::shared_ptr<Connection>& s) { return s.get() == c; });
		if (it == m_functions.end())
			return false;
		(*it)->m_connected = nullptr;
		m_functions.erase(it);
		return true;
	}

	inline void disconnectAll() {
		std::for_each(m_functions.begin(), m_functions.end(), [](const std::shared_ptr<Connection>& c) { c.m_connected = nullptr; });
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
	bool m_block{false};
};

} // namespace Zeeno

#endif
