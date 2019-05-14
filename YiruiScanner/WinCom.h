#pragma once
#include <queue>
#include <mutex>

typedef char byte;

template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<std::shared_ptr<T> > data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue() {}
	void wait_and_pop(T& value)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = std::move(*data_queue.front());
		data_queue.pop();
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = std::move(*data_queue.front());
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res = data_queue.front();
		data_queue.pop();
		return res;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res = data_queue.front();
		data_queue.pop();
		return res;
	}
	void push(T new_value)
	{
		std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(data);
		data_cond.notify_one();
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};

class data_info 
{
public:
	data_info() 
	{
		data = nullptr; len = 0;
	}
	data_info(byte* src, int s_len) 
	{
		data = new byte[s_len];
		memcpy(data, src, s_len);
		len = s_len;
	}
	~data_info() 
	{
		delete[] data; 
		data = nullptr; 
		len = 0;
	}

	byte* data;
	int   len;
};

typedef std::function<void(data_info*)> data_func;

class WinCom
{
public:
	WinCom();
	virtual ~WinCom();

	BOOL Open(const char* com_name, int baudRate);
	void Close();
	const char* Read();
	int  Write(PBYTE pData, int nDataLen);

	OVERLAPPED osRead;
	OVERLAPPED osWrite;
	OVERLAPPED ShareEvent;
	HANDLE COMFile;
	threadsafe_queue<data_info*> m_data_queue;
	bool is_com_close = false;
	data_func m_fn;
};