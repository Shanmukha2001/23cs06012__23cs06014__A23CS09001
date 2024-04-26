#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

// Define message types
enum class MessageType
{
    REQUEST,
    REPLY,
    RELEASE
};

// Message struct
struct Message
{
    MessageType type;
    int site_id;
    int timestamp;

    // Convert the message to a string for transmission
    string serialize() const
    {
        return to_string(static_cast<int>(type)) + "," + to_string(site_id) + "," + to_string(timestamp);
    }

    // Parse a string to create a Message object
    static Message deserialize(const std::string &s)
    {
        vector<string> parts;
        stringstream ss(s);
        string part;

        while (getline(ss, part, ','))
        {
            parts.push_back(part);
        }

        MessageType type = static_cast<MessageType>(stoi(parts[0]));
        int site_id = stoi(parts[1]);
        int timestamp = stoi(parts[2]);

        return {type, site_id, timestamp};
    }
};

// LamportMutexSystem class definition
class LamportMutexSystem
{
public:
    int system_id;
    int listening_port;
    vector<pair<int, string>> other_systems; // <system_id, ip:port>
    int logical_clock;
    bool f1 = false, f2 = false;
    mutex mtx;
    condition_variable cv;
    boost::asio::io_service io_service;
    vector<int> request_queue; // For REQUEST messages

    LamportMutexSystem(int id, int port, vector<pair<int, string>> systems)
        : system_id(id), listening_port(port), other_systems(systems), logical_clock(0) {}

    // Function to send mutual exclusion requests
    void sender_thread()
    {
        while (true)
        {
            // Simulate local events
            this_thread::sleep_for(chrono::seconds(4)); // Event interval (can be adjusted)
            request_critical_section();
        }
    }

    // Function to listen for incoming requests and handle them
    void receiver_thread()
    {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), listening_port));

        try
        {
            cout << "System " << system_id << " receiver thread is running and listening on port " << listening_port << endl;

            while (true)
            {
                tcp::socket socket(io_service);
                acceptor.accept(socket);

                // Receive message
                char buffer[256];
                size_t length = socket.read_some(boost::asio::buffer(buffer, 256)); // Limit the read size to avoid buffer overflow
                buffer[length] = '\0';                                              // Null-terminate the buffer

                // Parse incoming message
                Message incoming_message = Message::deserialize(buffer);

                unique_lock<mutex> lock(mtx);
                // Handle the received message based on its type
                handle_message(incoming_message);
            }
        }
        catch (const std::exception &e)
        {
            cerr << "Exception in receiver thread: " << e.what() << endl;
            acceptor.close();
        }
    }

    // Function to handle different types of messages
    void handle_message(const Message &msg)
    {
        // Update logical clock
        logical_clock = max(logical_clock, msg.timestamp) + 1;

        switch (msg.type)
        {
        case MessageType::REQUEST:
            handle_request(msg);
            break;
        case MessageType::REPLY:
            handle_reply(msg);
            break;
        case MessageType::RELEASE:
            handle_release(msg);
            break;
        }
    }

    // Function to handle REQUEST messages
    void handle_request(const Message &msg)
    {
        cout << "System " << system_id << " received a REQUEST message from System " << msg.site_id << " with timestamp " << msg.timestamp << endl;

        // Update logical clock
        logical_clock++;

        // Add to request queue
        request_queue.push_back(msg.site_id);

        // Sort request queue
        sort(request_queue.begin(), request_queue.end());

        // Send REPLY message back to the system with the least timestamp in the queue
        if (!f1 || !f2)
        {
            for (auto &system : other_systems)
            {
                if (system.first == msg.site_id)
                {
                    send_message_to_site(system.second, {MessageType::REPLY, system_id, logical_clock});
                    break;
                }
            }
        }
    }

    // Function to handle REPLY messages
    void handle_reply(const Message &msg)
    {
        cout << "System " << system_id << " received a REPLY message from System " << msg.site_id << " with timestamp " << msg.timestamp << endl;

        if (system_id == 1 && msg.site_id == 2)
        {
            f1 = true;
        }

        if (system_id == 1 && msg.site_id == 3)
        {
            f2 = true;
        }

        if (system_id == 2 && msg.site_id == 1)
        {
            f1 = true;
        }

        if (system_id == 2 && msg.site_id == 3)
        {
            f2 = true;
        }

        if (system_id == 3 && msg.site_id == 1)
        {
            f1 = true;
        }

        if (system_id == 3 && msg.site_id == 2)
        {
            f2 = true;
        }

        if (f1 && f2)
        {
            cout << "System " << system_id << " Entering Critical Section..." << endl;
            enter_critical_section();
        }
    }

    // Function to handle RELEASE messages
    void handle_release(const Message &msg)
    {
        cout << "System " << system_id << " received a RELEASE message from System " << msg.site_id << " with timestamp " << msg.timestamp << endl;

        // Update logical clock
        logical_clock = max(logical_clock, msg.timestamp) + 1;

        // Remove request from queue
        request_queue.erase(remove(request_queue.begin(), request_queue.end(), msg.site_id), request_queue.end());
    }

    // Function to send a mutual exclusion request
    void request_critical_section()
    {
        // Update logical clock
        logical_clock++;

        // Add to request queue
        request_queue.push_back(system_id);

        // Sort request queue
        sort(request_queue.begin(), request_queue.end());

        // Send REQUEST message to all other systems
        for (auto &system : other_systems)
        {
            if (system.first != system_id)
            {
                send_message_to_site(system.second, {MessageType::REQUEST, system_id, logical_clock});
            }
        }
    }

    // Function to enter the critical section
    void enter_critical_section()
    {
        cout << "System " << system_id << " is entering the critical section at logical clock " << logical_clock << endl;

        // Simulate critical section operation
        this_thread::sleep_for(chrono::seconds(2)); // Critical section duration (2 seconds)

        cout << "System " << system_id << " is leaving the critical section at logical clock " << logical_clock << endl;

        // Send RELEASE message to all other systems
        for (auto &system : other_systems)
        {
            if (system.first != system_id)
            {
                send_message_to_site(system.second, {MessageType::RELEASE, system_id, logical_clock});
            }
        }
    }

    // Function to send a message to a specific site
    void send_message_to_site(const string &address, const Message &msg)
    {
        try
        {
            size_t pos = address.find(":");
            string ip = address.substr(0, pos);
            int port = stoi(address.substr(pos + 1));

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(ip, to_string(port));
            tcp::resolver::iterator iterator = resolver.resolve(query);

            // Connect to the target site
            tcp::socket socket(io_service);
            boost::asio::connect(socket, iterator);

            // Send the message
            string serialized_message = msg.serialize();
            boost::asio::write(socket, boost::asio::buffer(serialized_message));
        }
        catch (const boost::system::system_error &e)
        {
            if (e.code() == boost::asio::error::connection_refused)
            {
                cerr << "Failed to send message to site: " << address << endl;
            }
            else
            {
                cerr << "Exception in send_message_to_site: " << e.what() << endl;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 9)
    {
        cerr << "Usage: " << argv[0] << " <system_id> <listening_port> <other_system_id1> <other_system_ip1> <other_system_port1> <other_system_id2> <other_system_ip2> <other_system_port2> <other_system_id3> <other_system_ip3> <other_system_port3>" << endl;
        return 1;
    }

    int system_id = stoi(argv[1]);
    int listening_port = stoi(argv[2]);
    vector<pair<int, string>> other_systems;

    for (int i = 3; i < argc; i += 3)
    {
        int other_id = stoi(argv[i]);
        string other_ip = argv[i + 1];
        int other_port = stoi(argv[i + 2]);
        other_systems.push_back({other_id, other_ip + ":" + to_string(other_port)});
    }

    // Create LamportMutexSystem object
    LamportMutexSystem lamport_mutex_system(system_id, listening_port, other_systems);

    // Start sender and receiver threads
    thread sender(&LamportMutexSystem::sender_thread, &lamport_mutex_system);
    thread receiver(&LamportMutexSystem::receiver_thread, &lamport_mutex_system);

    // Join threads
    sender.join();
    receiver.join();

    return 0;
}
