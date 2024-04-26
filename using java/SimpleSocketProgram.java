import java.io.*;
import java.net.*;
import java.util.*;

public class SimpleSocketProgram {
    private static final Object lock = new Object();
    private static PriorityQueue<RequestObj> requests;
    private static String criticalProviderIP;
    private static int criticalProviderPort;

    public static void main(String[] args) {
        try {
            Scanner sc = new Scanner(System.in);
            System.out.println("Enter your IP address:");
            String myIP = sc.nextLine();
            System.out.println("Enter your port number:");
            int myPort = sc.nextInt();
            requests = new PriorityQueue<>(Comparator.comparingLong(RequestObj::getTimestamp));
            Thread server = new Thread(new Server(myIP, myPort));
            server.start();

            System.out.println("Enter peer2 IP address:");
            sc.nextLine();
            String peer2IP = sc.nextLine();
            System.out.println("Enter peer2 port number:");
            int peer2Port = sc.nextInt();

            System.out.println("Enter peer3 IP address:");
            sc.nextLine();
            String peer3IP = sc.nextLine();
            System.out.println("Enter peer3 port number:");
            int peer3Port = sc.nextInt();

            System.out.println("Enter the IP address of the critical section provider:");
            sc.nextLine();
            criticalProviderIP = sc.nextLine();
            System.out.println("Enter the port number of the critical section provider:");
            criticalProviderPort = sc.nextInt();

            Thread myClient = new Thread(new Client(peer2IP, peer2Port, peer3IP, peer3Port));
            myClient.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static class RequestObj {
        private Long timestamp;
        private int port;

        public RequestObj(Long timestamp, int port) {
            this.timestamp = timestamp;
            this.port = port;
        }

        public Long getTimestamp() {
            return timestamp;
        }

        public int getPort() {
            return port;
        }
    }

    static class Client implements Runnable {
        private String peer2IP, peer3IP;
        private int peer2Port, peer3Port;

        Client(String peer2IP, int peer2Port, String peer3IP, int peer3Port) {
            this.peer2IP = peer2IP;
            this.peer2Port = peer2Port;
            this.peer3IP = peer3IP;
            this.peer3Port = peer3Port;
        }

        @Override
        public void run() {
            try {
                Socket peer2 = new Socket(peer2IP, peer2Port);
                Socket peer3 = new Socket(peer3IP, peer3Port);
                PrintWriter out2 = new PrintWriter(peer2.getOutputStream(), true);
                BufferedReader in2 = new BufferedReader(new InputStreamReader(peer2.getInputStream()));
                PrintWriter out3 = new PrintWriter(peer3.getOutputStream(), true);
                BufferedReader in3 = new BufferedReader(new InputStreamReader(peer3.getInputStream()));

                while (true) {
                    out2.println("CRITICAL__" + System.currentTimeMillis());
                    out3.println("CRITICAL__" + System.currentTimeMillis());
                    
                    synchronized (lock) {
                        while (requests.size() < 2) {
                            lock.wait();
                        }
                        Socket criticalProvider = new Socket(criticalProviderIP, criticalProviderPort);
                        PrintWriter out = new PrintWriter(criticalProvider.getOutputStream(), true);
                        out.println("GRANTED_ACCESS_TO_CRITICAL_SECTION");
                        out.close();
                        criticalProvider.close();
                        requests.clear();
                    }

                    Thread.sleep(3000);
                }
            } catch (IOException | InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    static class Server implements Runnable {
        private String ip;
        private int port;

        Server(String ip, int port) {
            this.ip = ip;
            this.port = port;
        }

        @Override
        public void run() {
            try {
                ServerSocket serverSocket = new ServerSocket(port, 50, InetAddress.getByName(ip));
                System.out.println("Server started on " + ip + ":" + port + ". Waiting for client to connect...");

                while (true) {
                    Socket clientSocket = serverSocket.accept();
                    System.out.println("Client connected.");

                    Thread clientHandler = new Thread(new ClientHandler(clientSocket));
                    clientHandler.start();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

   static class ClientHandler implements Runnable {
    private Socket clientSocket;

    ClientHandler(Socket clientSocket) {
        this.clientSocket = clientSocket;
    }

    private void accessCriticalSection() {
        // Comment out the println and write to file instead
        try {
            FileWriter writer = new FileWriter("critical_section.txt", true);
            writer.write("Accessing Critical Section...\n");
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        try {
            PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));

            String message;
            while ((message = in.readLine()) != null) {
                if (message.startsWith("NORMAL")) {
                    System.out.println(message.split("__")[1]);
                } else if (message.startsWith("CRITICAL")) {
                    Long receivedTimestamp = Long.parseLong(message.split("__")[1]);
                    synchronized (lock) {
                        requests.offer(new RequestObj(receivedTimestamp, clientSocket.getPort()));
                        lock.notifyAll();
                    }
                } else if (message.startsWith("GRANTED_ACCESS_TO_CRITICAL_SECTION")) {
                    System.out.println("Access to critical section granted by the provider. Socket: " + clientSocket.getPort());
                    synchronized (lock) {
                        Iterator<RequestObj> iterator = requests.iterator();
                        while (iterator.hasNext()) {
                            RequestObj request = iterator.next();
                            if (request.getPort() == clientSocket.getPort()) {
                                iterator.remove();
                                lock.notifyAll(); // Notify other threads to remove from their list
                                break;
                            }
                        }
                    }
                    accessCriticalSection();
                }
                System.out.println("Received message from client: " + message + " port:" + clientSocket.getPort());
            }

            out.close();
            in.close();
            clientSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
}