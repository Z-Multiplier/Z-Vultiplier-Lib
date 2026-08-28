#include "Initializer.hpp"
#include "Handle.hpp"
#include "Clock.hpp"
#include "Network.hpp"
#include "Logger.hpp"

class MyServer:public Network::Server{
public:
    MyServer()=default;

protected:
    void OnConnect(ENetPeer* peer)override{
        Network::Server::OnConnect(peer);
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Server] Client connected! ID: "+std::to_string(peer->connectID),
            std::source_location::current()
        );
    }

    void OnReceive(ENetPeer* peer,const uint8_t* data,size_t len)override{
        Network::BitStream stream(std::vector<uint8_t>(data,data+len));
        uint32_t msgType=stream.ReadUInt();

        if(msgType==1){
            std::string text=stream.ReadString();
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "[Server] Received: "+text,
                std::source_location::current()
            );

            Network::BitStream reply;
            reply.WriteUInt(2);
            reply.WriteString("Echo: "+text);
            Broadcast(reply.GetData());
        }
    }

    void OnDisconnect(ENetPeer* peer)override{
        Network::Server::OnDisconnect(peer);
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Server] Client disconnected! ID: "+std::to_string(peer->connectID),
            std::source_location::current()
        );
    }
};

class MyClient:public Network::Client{
public:
    MyClient()=default;

    void SendMessage(const std::string& text){
        if(!IsConnected()){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "[Client] Not connected,cannot send.",
                std::source_location::current()
            );
            return;
        }
        Network::BitStream stream;
        stream.WriteUInt(1);
        stream.WriteString(text);
        sendToServer(stream.GetData());
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Client] Sent: "+text,
            std::source_location::current()
        );
    }

    bool IsConnected()const{
        return serverConn!=nullptr;
    }

protected:
    void onServerMessage(const uint8_t* data,size_t len)override{
        Network::BitStream stream(std::vector<uint8_t>(data,data+len));
        uint32_t msgType=stream.ReadUInt();

        if(msgType==2){
            std::string reply=stream.ReadString();
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "[Client] Received: "+reply,
                std::source_location::current()
            );
        }
    }
};

int main(){
    freopen("console.log","w",stderr);
    try{
        Core::Initializer init("Network Test");
        auto& manager=Window::WindowManager::instance();
        auto window=manager.create(init,800,600,"Network Test (Press SPACE to send)");

        MyServer server;
        server.Start(8888,4);

        bool serverRunning=true;
        std::thread serverThread([&](){
            while(serverRunning){
                server.Update(0.016f);
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        });

        MyClient client;
        client.connect("127.0.0.1",8888);

        int messageCount=0;
        window->setKeyCallback([&](int key,int scancode,int action,int mods){
            if(action==GLFW_PRESS&&key==GLFW_KEY_SPACE){
                messageCount++;
                std::string msg="Hello #"+std::to_string(messageCount);
                client.SendMessage(msg);
            }
        });

        Core::Clock clock(nullptr,60);
        while(clock){
            clock.run();

            client.update(0.016f);

            static bool connectedLogged=false;
            if(client.IsConnected()&&!connectedLogged){
                Core::globalLogger.traceLog(
                    Core::logger::LOG_INFO,
                    "[Main] Client connected to server!",
                    std::source_location::current()
                );
                connectedLogged=true;
            }

            if(window->shouldClose()){
                break;
            }
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Main] Shutting down...",
            std::source_location::current()
        );

        client.disconnect();

        for(int i=0;i<10;i++){
            client.update(0.016f);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        serverRunning=false;
        serverThread.join();

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Main] Network test completed.",
            std::source_location::current()
        );

    }catch(const std::exception& e){
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}