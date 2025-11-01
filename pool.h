#pragma once
#include "handle.h"

// TODO: make an iterator

template <typename T>
class Pool {
public:
	const T* data() const { return _data.data(); }

	size_t size() const { return _data.size(); }

	std::span<T> span() { return { _data.data(), _data.size() }; }

	void clear();

	template <typename... Args>
	Handle<T> emplace(Args&&... args);

	bool valid(Handle<T> handle);

	T* get(Handle<T> handle);

	void free(Handle<T> handle);

private:
	eastl::vector<T> _data;
	eastl::vector<uint16_t> _versions; // Same size as _data.
	eastl::vector<uint16_t> _freelist; // Stores the indices of previously freed slots.
};

template<typename T>
inline void Pool<T>::clear() {
	_data.clear();
	_versions.clear();
	_freelist.clear();
}

template<typename T>
template<typename ...Args>
inline Handle<T> Pool<T>::emplace(Args&&... args) {
	Handle<T> handle;
	if (_freelist.empty()) {
		handle.index = (uint16_t)_data.size();
		handle.version = 1;  // Valid generations start at 1.
		_versions.push_back(handle.version);
		_data.emplace_back(std::forward<Args>(args)...);
	} else {
		handle.index = _freelist.back();
		_freelist.pop_back();
		handle.version = _versions[handle.index];
		// If T supports move assignment, use it. Otherwise, destroy and reconstruct in place.
		if constexpr (std::is_move_assignable_v<T>) {
			_data[handle.index] = T(std::forward<Args>(args)...);
		} else {
			_data[handle.index].~T();
			new (&_data[handle.index]) T(std::forward<Args>(args)...);
		}
	}
	return handle;
}

template<typename T>
inline bool Pool<T>::valid(Handle<T> handle) {
	if (handle.index >= _data.size())
		return false;
	if (handle.version != _versions[handle.index])
		return false;
	return true;
}

template<typename T>
inline T* Pool<T>::get(Handle<T> handle) {
	if (!valid(handle))
		return nullptr;
	return &_data[handle.index];
}

// PITFALL: Doesn't deconstruct or otherwise cleanup the element! You need to do that yourself if necessary.
template<typename T>
inline void Pool<T>::free(Handle<T> handle) {
	if (!valid(handle))
		return;
	_versions[handle.index]++;
	_freelist.push_back(handle.index);
}
