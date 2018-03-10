#pragma once

class ProtocolHandler;

// HTTP Wrapper around a uv_tcp_t
class InspectorSocket {
public:
	class Delegate {
	public:
		virtual void OnHttpGet(const std::string& path) = 0;
		virtual void OnSocketUpgrade(const std::string& path,
			const std::string& accept_key) = 0;
		virtual void OnWsFrame(const std::vector<char>& frame) = 0;
		virtual ~Delegate() {}
	};

	using DelegatePointer = std::unique_ptr<Delegate>;
	using Pointer = std::unique_ptr<InspectorSocket>;

	static Pointer Accept(uv_stream_t* server, DelegatePointer delegate);

	~InspectorSocket();

	void AcceptUpgrade(const std::string& accept_key);
	void CancelHandshake();
	void Write(const char* data, size_t len);
	void SwitchProtocol(ProtocolHandler* handler);
	std::string GetHost();

private:
	InspectorSocket();

	std::unique_ptr<ProtocolHandler, void(*)(ProtocolHandler*)> protocol_handler_;

};

#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#define PRETTY_FUNCTION_NAME ""

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define CHECK(expr)                                                           \
  do {                                                                        \
    if (UNLIKELY(!(expr))) {                                                  \
      static const char* const args[] = { __FILE__, STRINGIFY(__LINE__),      \
                                          #expr, PRETTY_FUNCTION_NAME };      \
      Assert(&args);                                                    \
    }                                                                         \
  } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_NE(a, b) CHECK((a) != (b))

#define base64_encoded_size(size) ((size + 2 - ((size + 2) % 3)) / 3 * 4)

#define ACCEPT_KEY_LENGTH base64_encoded_size(20)

#define NO_RETURN

NO_RETURN void Assert(const char* const (*args)[4]);

bool StringEqualNoCaseN(const char* a, const char* b, size_t length);

template <typename Inner, typename Outer>
class ContainerOfHelper {
public:
	inline ContainerOfHelper(Inner Outer::*field, Inner* pointer);
	template <typename TypeName>
	inline operator TypeName*() const;
private:
	Outer * const pointer_;
};

// Calculate the address of the outer (i.e. embedding) struct from
// the interior pointer to a data member.


template <typename Inner, typename Outer>
inline ContainerOfHelper<Inner, Outer> ContainerOf(Inner Outer::*field,
	Inner* pointer);


template <typename Inner, typename Outer>
constexpr uintptr_t OffsetOf(Inner Outer::*field) {
	return reinterpret_cast<uintptr_t>(&(static_cast<Outer*>(0)->*field));
}

template <typename Inner, typename Outer>
ContainerOfHelper<Inner, Outer>::ContainerOfHelper(Inner Outer::*field,
	Inner* pointer)
	: pointer_(
		reinterpret_cast<Outer*>(
			reinterpret_cast<uintptr_t>(pointer) - OffsetOf(field))) {}

template <typename Inner, typename Outer>
template <typename TypeName>
ContainerOfHelper<Inner, Outer>::operator TypeName*() const {
	return static_cast<TypeName*>(pointer_);
}

template <typename Inner, typename Outer>
inline ContainerOfHelper<Inner, Outer> ContainerOf(Inner Outer::*field,
	Inner* pointer) {
	return ContainerOfHelper<Inner, Outer>(field, pointer);
}

class Closer;
class SocketSession;
class ServerSocket;

class SocketServerDelegate {
public:
	virtual void StartSession(int session_id, const std::string& target_id) = 0;
	virtual void EndSession(int session_id) = 0;
	virtual void MessageReceived(int session_id, const std::string& message) = 0;
	virtual std::vector<std::string> GetTargetIds() = 0;
	virtual std::string GetTargetTitle(const std::string& id) = 0;
	virtual std::string GetTargetUrl(const std::string& id) = 0;
	virtual void ServerDone() = 0;
};

// HTTP Server, writes messages requested as TransportActions, and responds
// to HTTP requests and WS upgrades.

class InspectorSocketServer {
public:
	using ServerCallback = void(*)(InspectorSocketServer*);
	InspectorSocketServer(SocketServerDelegate* delegate,
		uv_loop_t* loop,
		const std::string& host,
		int port,
		FILE* out = stderr);
	~InspectorSocketServer();

	// Start listening on host/port
	bool Start();

	// Called by the TransportAction sent with InspectorIo::Write():
	//   kKill and kStop
	void Stop(ServerCallback callback);
	//   kSendMessage
	void Send(int session_id, const std::string& message);
	//   kKill
	void TerminateConnections();
	//   kAcceptSession
	void AcceptSession(int session_id);
	//   kDeclineSession
	void DeclineSession(int session_id);

	int Port() const;

	// Server socket lifecycle. There may be multiple sockets
	void ServerSocketListening(ServerSocket* server_socket);
	void ServerSocketClosed(ServerSocket* server_socket);

	// Session connection lifecycle
	void Accept(int server_port, uv_stream_t* server_socket);
	bool HandleGetRequest(int session_id, const std::string& path);
	void SessionStarted(int session_id, const std::string& target_id,
		const std::string& ws_id);
	void SessionTerminated(int session_id);
	void MessageReceived(int session_id, const std::string& message) {
		delegate_->MessageReceived(session_id, message);
	}
	SocketSession* Session(int session_id);

private:
	void SendListResponse(InspectorSocket* socket, SocketSession* session);
	bool TargetExists(const std::string& id);

	enum class ServerState { kNew, kRunning, kStopping, kStopped };
	uv_loop_t* loop_;
	SocketServerDelegate* const delegate_;
	const std::string host_;
	int port_;
	std::string path_;
	std::vector<ServerSocket*> server_sockets_;
	Closer* closer_;
	std::map<int, std::pair<std::string, std::unique_ptr<SocketSession>>>
		connected_sessions_;
	int next_session_id_;
	FILE* out_;
	ServerState state_;

	friend class Closer;
};

