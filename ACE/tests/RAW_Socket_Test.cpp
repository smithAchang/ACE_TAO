// ============================================================================
// ============================================================================
//
// = LIBRARY
//    tests
//
// = DESCRIPTION
//    This program tests the basic APIs supported in
//    <ACE_RAW_Socket>, and demonstrates how to use it.
//
// = AUTHOR
//    Smith Achang <changyunlei@126.com>
//
// ============================================================================

#include "test_config.h"
#include "ace/RAW_Socket.h"
#include "ace/SOCK_Dgram.h"

typedef struct _IP_HEADER_t
{
   uint8_t  bVersionAndHeaderLen;      // 版本信息(前4位)，头长度(后4位)
   uint8_t  bTypeOfService;           // 服务类型8位
   uint16_t u16TotalLenOfPacket;     // 数据包长度
   uint16_t u16PacketID;            // 数据包标识
   uint16_t u16Sliceinfo;          // 分片使用
   uint8_t  bTTL;                 // 存活时间
   uint8_t  bTypeOfProtocol;     // 协议类型
   uint16_t u16CheckSum;        // 校验和
   uint32_t u32SourIp;         // 源ip
   uint32_t u32DestIp;        // 目的ip
} __attribute__((packed)) IPv4_HEADER_t, *IPv4_HEADER_t_Ptr;

typedef struct _IPv6_HEADER_t
{
   union
   {
      uint8_t abVTF[4];  // 版本信息 most top 4 bits , traffic 8 bits
      uint32_t dwVTF;   // flow label lefted 20 bits;
   };

   uint16_t u16PayloadLen; // 数据包长度
   uint8_t  bNextHeader;  // 数据包标识
   uint8_t  bHopLimit;   // max hop limit

   union
   {
      uint8_t  abSrcAddr[16];
      uint32_t au32SrcAddr[4];
      uint64_t au64SrcAddr[2];
   };

   union
   {
      uint8_t  abDstAddr[16];
      uint32_t au32DstAddr[4];
      uint64_t au64DstAddr[2];
   };

} __attribute__((packed)) IPv6_HEADER_t, *IPv6_HEADER_t_Ptr;

typedef struct _UDP_HEADER_t
{
   uint16_t u16SrcPort;
   uint16_t u16DstPort;
   uint16_t u16Length;
   uint16_t u16CheckSum;
} __attribute__((packed)) UDP_HEADER_t, *UDP_HEADER_t_Ptr;


class SockGuard : private ACE_Copy_Disabled 
{ 
public:
SockGuard(ACE_SOCK& sock):sock_(sock){};
~SockGuard(){ sock_.close(); };

private:
 ACE_SOCK& sock_;
};


#define EXCEPTION_RETURN(expression, message)\
do{\
    if(expression)\
    {\
      ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT (message)), 1);\
}\
}while(0)

static char  sendbuf[4096];
static char  recvbuf[4096];

/*
 * This is the heart of the test. It test the api of RAW socket class
 *
 * It returns 0 for success and 1 for error so we can keep track of the
 * total error count.
 */


static int
run_option_test ()
{
 
  ACE_DEBUG ((LM_INFO, "%s begin to run ...\n", __func__));

  ACE_INET_Addr addr(u_short(0), "127.0.0.1");
  ACE_RAW_SOCKET  rawSocket(addr);
  SockGuard guard(rawSocket);

  EXCEPTION_RETURN(rawSocket.get_handle() == ACE_INVALID_HANDLE, "  can not bind the addr\n");

  int rc = rawSocket.enable(ACE_NONBLOCK);

  EXCEPTION_RETURN(rc < 0, "  can not bind the addr\n");


  int optval = 0, optlen = sizeof(optval);
  rc = rawSocket.get_option(SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
  
  EXCEPTION_RETURN(rc < 0, "  get SO_RCVBUF in failure\n");

  optlen  = sizeof(optval);
  int new_optval = optval << 1;
  rc = rawSocket.set_option(SOL_SOCKET, SO_RCVBUF, &new_optval, sizeof(new_optval));

  EXCEPTION_RETURN(rc < 0, "  set SO_RCVBUF new value in failure\n");

  new_optval = 0;
  optlen  = sizeof(new_optval);

  rawSocket.get_option(SOL_SOCKET, SO_RCVBUF, &new_optval, &optlen);

  EXCEPTION_RETURN(new_optval < optval, "  set SO_RCVBUF with no effect\n");

  ACE_DEBUG ((LM_INFO, "old optval: %d, new optval ...\n", optval, new_optval));

  return 0;
}


static int
run_reopen_test ()
{
 
  ACE_DEBUG ((LM_INFO, "%s begin to run ...\n", __func__));
  
  ACE_INET_Addr addr((u_short)0, "127.0.0.1");
  ACE_RAW_SOCKET  rawSocket(addr);
  SockGuard guard(rawSocket);

  EXCEPTION_RETURN(rawSocket.get_handle() == ACE_INVALID_HANDLE, "  can not bind the addr\n");


  rawSocket.close();

  EXCEPTION_RETURN(rawSocket.get_handle() != ACE_INVALID_HANDLE, "  close in failure\n");

  ACE_INET_Addr addr2((u_short)0, "127.0.0.8");
  int rc = rawSocket.open(addr2);

  EXCEPTION_RETURN(rc < 0, "  reopen in failue\n");
  EXCEPTION_RETURN(rawSocket.get_handle() == ACE_INVALID_HANDLE, "  handle is invalid after re-open\n");
 

  return 0;
}

static int raw_recv_data_until_meet_condition(ACE_RAW_SOCKET& raw, u_short port, size_t n, ACE_INET_Addr& remote, ACE_INET_Addr* to_addr = nullptr)
{
   ACE_INET_Addr local;
   raw.get_local_addr(local);

   ssize_t len = 0;
   ssize_t expectedLen;

   do
   {
     
     if(to_addr == nullptr)
     {
         len = raw.recv(recvbuf, sizeof(recvbuf), remote); 
     }
     else
     {
         len = raw.recv(recvbuf, sizeof(recvbuf), remote, 0/*flags*/, nullptr, to_addr); 
     }
     

     if(len < 0)
     {
         ACE_DEBUG ((LM_INFO, "%s IPv6 recv expected pkgs ...\n", __func__));
         return -1;
     }

     UDP_HEADER_t_Ptr  ptUDPHeader;
     if(local.get_type() == AF_INET)
     {
       ptUDPHeader  = (UDP_HEADER_t_Ptr)(recvbuf + sizeof(IPv4_HEADER_t));
       expectedLen  = (n + sizeof(IPv4_HEADER_t) + sizeof(UDP_HEADER_t));
       u_short nDstPort    = ntohs(ptUDPHeader->u16DstPort);

       if(port == nDstPort && len == expectedLen)
       {
        ACE_DEBUG ((LM_INFO, "%s IPv4 recv expected pkgs ...\n", __func__));
        break;
       }
     }
     else
     {
       ptUDPHeader         = (UDP_HEADER_t_Ptr)recvbuf;
       expectedLen         = (n + sizeof(UDP_HEADER_t));
       u_short nDstPort    = ntohs(ptUDPHeader->u16DstPort);

       if(port == nDstPort && len == expectedLen)
       {
        ACE_DEBUG ((LM_INFO, "%s IPv6 recv expected pkgs ...\n", __func__));
        break;
       }
     }
     
     ACE_DEBUG ((LM_DEBUG, "%s recv unexpected pkgs len: %d, expectedLen: %d ...\n", __func__, len, expectedLen));
     ACE_OS::sleep(1);
   } while (1);
   
  return 0;
}

static int
run_raw_udp_test_child_flow_sendby_self (ACE_RAW_SOCKET& raw, ACE_INET_Addr& client_addr, ACE_INET_Addr& server_addr, size_t n)
{
   ACE_DEBUG ((LM_INFO, "%s begin to run when sending data by self ...\n", __func__));

   UDP_HEADER_t_Ptr   ptUDPHeader  = (UDP_HEADER_t_Ptr)(sendbuf + sizeof(IPv4_HEADER_t_Ptr));

   ptUDPHeader->u16SrcPort  = htons(client_addr.get_port_number());
   ptUDPHeader->u16DstPort  = htons(server_addr.get_port_number());
   ptUDPHeader->u16Length   = htons(n + sizeof(UDP_HEADER_t));
   ptUDPHeader->u16CheckSum = 0;

   int rc = raw.send(ptUDPHeader, n + sizeof(UDP_HEADER_t), server_addr);
   ssize_t expectedLen = n + sizeof(UDP_HEADER_t);
   EXCEPTION_RETURN(rc != expectedLen, "  raw socket can not send test pkg to server\n");

   u_short server_port = server_addr.get_port_number();
   ACE_INET_Addr remote;
   rc = raw_recv_data_until_meet_condition(raw, server_port, n, remote);
   EXCEPTION_RETURN(rc != 0, "  can recv test pkg from raw socket\n");
   
   return 0;
}

static int
run_raw_udp_test ()
{
  ACE_DEBUG ((LM_INFO, "%s begin to run using the port auto assigned by OS to avoid port conflict ...\n", __func__));

  ACE_INET_Addr addr((u_short)0, "127.0.0.1");
  
  ACE_SOCK_Dgram  dgram(addr);
  SockGuard dgram_guard(dgram);

  ACE_SOCK_Dgram  dgram_server(addr);
  SockGuard dgram_server_guard(dgram_server);

  ACE_RAW_SOCKET  rawSocket(addr);
  SockGuard raw_guard(rawSocket);

  ACE_INET_Addr client_addr ,server_addr,remote;
  int rc = dgram.get_local_addr (client_addr);

  EXCEPTION_RETURN(rc < 0, "  can not get client bound address\n");

  rc     = dgram_server.get_local_addr (server_addr);

  EXCEPTION_RETURN(rc < 0, "  can not get server address\n");

  ssize_t n = 512;
  rc = dgram.send(sendbuf, n, server_addr);
  EXCEPTION_RETURN(rc != n, "  can send test pkg to server\n");

  u_short server_port = server_addr.get_port_number();
  rc = raw_recv_data_until_meet_condition(rawSocket, server_port, n, remote);

  EXCEPTION_RETURN(rc != 0, "  can recv test pkg from raw socket\n");

  rc = run_raw_udp_test_child_flow_sendby_self (rawSocket, client_addr, server_addr, n + 1);
  EXCEPTION_RETURN(rc != 0, "  can recv test pkg from raw socket when sending by self\n");

  if(ACE_OS::getuid() == 0)
  {
      ACE_DEBUG ((LM_INFO, "%s test send & recv big pkt ...\n", __func__));
      rc = run_raw_udp_test_child_flow_sendby_self (rawSocket, client_addr, server_addr, n + 2048);
      EXCEPTION_RETURN(rc != 0, "  can recv test pkg from raw socket when sending big pkg by self\n");
  }
    
  return 0;
}

static int
run_raw_generic_test ()
{
   ACE_DEBUG ((LM_INFO, "%s begin to run generic raw socket i.e. send only RAW socket  ...\n", __func__));

   ACE_INET_Addr bindAddr((u_short)0, "127.0.0.1"), remote;
   ACE_INET_Addr client_addr((u_short)0, "127.0.0.7") ,server_addr((u_short)0, "127.0.0.8");
   ACE_SOCK_Dgram  client_dgram(client_addr);
   SockGuard client_dgram_guard(client_dgram);

   ACE_SOCK_Dgram  server_dgram(server_addr);
   SockGuard server_dgram_guard(server_dgram);

   ACE_RAW_SOCKET  rawSocket(bindAddr, IPPROTO_RAW);
   SockGuard raw_guard(rawSocket);

   EXCEPTION_RETURN(rawSocket.is_send_only() == false, "  raw socket is not send only\n");

   ssize_t len = rawSocket.recv(recvbuf, sizeof(recvbuf),  remote);
   EXCEPTION_RETURN(len  != -1, "  raw generic socket is send only , must not can recv data\n");

  
   client_dgram.get_local_addr (client_addr);
   server_dgram.get_local_addr (server_addr);

   IPv4_HEADER_t_Ptr  ptIPv4Header    = (IPv4_HEADER_t_Ptr)sendbuf;
   UDP_HEADER_t_Ptr   ptUDPHeader     = (UDP_HEADER_t_Ptr)(sendbuf + sizeof(IPv4_HEADER_t));
   u_short n = 2048;

   *ptIPv4Header = {};

   ptIPv4Header->bVersionAndHeaderLen = 0x45;
   ptIPv4Header->u16TotalLenOfPacket  = htons(sizeof(IPv4_HEADER_t) + sizeof(UDP_HEADER_t) + n);     // 数据包长度
   ptIPv4Header->u16PacketID          = ACE_OS::rand();  
   ptIPv4Header->bTTL                 = 64;
   ptIPv4Header->bTypeOfProtocol      = IPPROTO_UDP; 
   ptIPv4Header->u16CheckSum          = 0;
   ptIPv4Header->u32SourIp            = (static_cast<sockaddr_in*>(client_addr.get_addr()))->sin_addr.s_addr;
   ptIPv4Header->u32DestIp            = (static_cast<sockaddr_in*>(server_addr.get_addr()))->sin_addr.s_addr;

   u_short client_port_number = client_addr.get_port_number();
   u_short server_port_number = server_addr.get_port_number();


   ptUDPHeader->u16SrcPort  = htons(client_port_number);
   ptUDPHeader->u16DstPort  = htons(server_port_number);
   ptUDPHeader->u16Length   = htons(sizeof(UDP_HEADER_t) + n);
   ptUDPHeader->u16CheckSum = 0;

   if(ACE_OS::getuid() == 0)
   {
      ACE_DEBUG ((LM_INFO, "%s raw generic socket will send bytes exceeding the MTU  ...\n", __func__));
      n = 2048;
      len = rawSocket.send(sendbuf, sizeof(IPv4_HEADER_t) + sizeof(UDP_HEADER_t) + n,  server_addr);
      EXCEPTION_RETURN(len  != -1, "  raw generic socket can not send pkg more than MTU\n");
   }
   
   n = 468;
   ptUDPHeader->u16Length   = htons(sizeof(UDP_HEADER_t) + n);
   len = rawSocket.send(sendbuf, sizeof(IPv4_HEADER_t) + sizeof(UDP_HEADER_t) + n,  server_addr);
   size_t expectedLen = (sizeof(IPv4_HEADER_t) + sizeof(UDP_HEADER_t) + n);
   ACE_DEBUG ((LM_INFO, "%s raw generic socket send %d bytes, expected %u bytes  ...\n", __func__, len, expectedLen));
   EXCEPTION_RETURN(static_cast<size_t>(len)  != expectedLen, "  raw generic socket send pkg in failure\n");

   ACE_OS::sleep(1);
   ACE_DEBUG ((LM_INFO, "%s enable nonblock status ...\n", __func__));
   server_dgram.enable(ACE_NONBLOCK);
   len = server_dgram.recv(recvbuf, sizeof(recvbuf), remote);
   expectedLen =  n;
   ACE_DEBUG ((LM_INFO, "%s udp server socket recv %d bytes, expected %u bytes  ...\n", __func__, len, expectedLen));
   EXCEPTION_RETURN(static_cast<size_t>(len)  !=  n, "  server socket receives pkg in failure length is not the same\n");
   EXCEPTION_RETURN(static_cast<sockaddr_in*>(remote.get_addr())->sin_addr.s_addr  !=  static_cast<sockaddr_in*>(client_addr.get_addr())->sin_addr.s_addr, "  server socket receives pkg in failure: the source IP is not the same\n");
   
   return 0;
}

#if defined (ACE_HAS_IPV6)
static int
run_ipv6_pkginfo_test ()
{
   ACE_DEBUG ((LM_INFO, "%s begin to run IPv6 pkginfo test ...\n", __func__));

   ACE_INET_Addr bindAddr((u_short)0, "::1");
   ACE_INET_Addr anyAddr((u_short)0, "::");

   ACE_INET_Addr client_addr((u_short)0, "::1") ,server_addr((u_short)0, "::1");

   
   ACE_SOCK_Dgram  client_dgram(client_addr);
   SockGuard client_dgram_guard(client_dgram);
   client_dgram.get_local_addr(client_addr);

   ACE_SOCK_Dgram  server_dgram(server_addr);
   SockGuard server_dgram_guard(server_dgram);
   server_dgram.get_local_addr(server_addr);

   ACE_DEBUG ((LM_INFO, "%s get the real bound addr and port client_port: %u, server_port: %u...\n", __func__, client_addr.get_port_number(), server_addr.get_port_number()));

   

   ACE_RAW_SOCKET  rawSocket(bindAddr, IPPROTO_UDP);
   rawSocket.enable(ACE_NONBLOCK);
   SockGuard raw_guard(rawSocket);

   ACE_RAW_SOCKET  rawWildcardSocket(anyAddr, IPPROTO_UDP);
   rawWildcardSocket.enable(ACE_NONBLOCK);
   SockGuard raw_wildcard_guard(rawWildcardSocket);
   
   client_dgram.send("hello world", sizeof("hello world"), server_addr);
   
   ACE_DEBUG ((LM_INFO, "%s the send pkg will be received by two raw sockets ...\n", __func__));
   ACE_OS::sleep(1);

   ACE_INET_Addr remote;   
   int rc = raw_recv_data_until_meet_condition(rawSocket, server_addr.get_port_number(), sizeof("hello world"), remote);
   
   EXCEPTION_RETURN(rc != 0, "  non wildcard raw socket can not recv expectedRecvLen\n");

   ACE_INET_Addr to_addr;

   ACE_DEBUG ((LM_INFO, "%s send pkg again to test common raw socket with to_adr parameter ...\n", __func__));
   client_dgram.send("hello world", sizeof("hello world"), server_addr);
   ACE_OS::sleep(1);

   int yes = 1;
   
   rc = raw_recv_data_until_meet_condition(rawSocket, server_addr.get_port_number(), sizeof("hello world"), remote, &to_addr);
   EXCEPTION_RETURN(rc != 0, "  non wildcard raw socket can not recv expectedRecvLen with to_addr parameter\n");
    
   in6_addr* remote_sin6_addr = &(static_cast<sockaddr_in6 *>(remote.get_addr())->sin6_addr);
   in6_addr* to_sin6_addr     = &(static_cast<sockaddr_in6 *>(to_addr.get_addr())->sin6_addr);
   int cmp = ACE_OS::memcmp(remote_sin6_addr, to_sin6_addr, sizeof(*to_sin6_addr));
   EXCEPTION_RETURN(cmp != 0, "  non wildcard raw socket got to_addr with invalid value\n");

  
   rc = raw_recv_data_until_meet_condition(rawWildcardSocket, server_addr.get_port_number(), sizeof("hello world"), remote, &to_addr);
   EXCEPTION_RETURN(rc != 0, "  can not recv expectedRecvLen with to_addr when provided to wildcard RAW socket\n");
   remote_sin6_addr = &(static_cast<sockaddr_in6 *>(remote.get_addr())->sin6_addr);
   to_sin6_addr     = &(static_cast<sockaddr_in6 *>(to_addr.get_addr())->sin6_addr);
   cmp = ACE_OS::memcmp(remote_sin6_addr, to_sin6_addr, sizeof(*to_sin6_addr));
   EXCEPTION_RETURN(cmp != 0, "  to_addr with invalid value when provided to wildcard RAW socket\n");
       
   return 0;
}
#endif

int
run_main (int, ACE_TCHAR *argv[])
{
  ACE_START_TEST (ACE_TEXT ("RAW_Socket_Test"));
  ACE_UNUSED_ARG (argv);
  int retval = 0;
  int oldMTU = 1500;

  // set the lo interface MTU
  if(ACE_OS::getuid() == 0)
  {
    ACE_INET_Addr anyAddr((u_short)0);
    ACE_SOCK_Dgram  netdevice(anyAddr);
    SockGuard dgram_guard(netdevice);

    struct ifreq tReq = {};
    ACE_OS::snprintf(tReq.ifr_name, sizeof(tReq.ifr_name), "%s", "lo");
    tReq.ifr_mtu = 1400;
    ACE_OS::ioctl(netdevice.get_handle(), SIOCSIFMTU, &tReq);

    tReq.ifr_mtu = 0;
    ACE_OS::ioctl(netdevice.get_handle(), SIOCGIFMTU, &tReq);
    EXCEPTION_RETURN(tReq.ifr_mtu != 1400, "  can set MTU for lo interface\n");
  }
  
 

  // Run the tests for each type of ordering.
  retval = run_option_test ();
  retval += run_reopen_test();
  retval += run_raw_udp_test();
  retval += run_raw_generic_test();

  #if defined (ACE_HAS_IPV6)
  retval += run_ipv6_pkginfo_test();
  #elif
  ACE_DEBUG ((LM_INFO, "%s without IPv6 macro ...\n", __func__));
  #endif
  

  ACE_END_TEST;

 if(ACE_OS::getuid() == 0)
  {
    ACE_INET_Addr anyAddr((u_short)0);
    ACE_SOCK_Dgram  netdevice(anyAddr);
    SockGuard dgram_guard(netdevice);

    struct ifreq tReq = {};
    ACE_OS::snprintf(tReq.ifr_name, sizeof(tReq.ifr_name), "%s", "lo");
    tReq.ifr_mtu = oldMTU;
    ACE_OS::ioctl(netdevice.get_handle(), SIOCSIFMTU, &tReq);
  }
  

  return retval;
}
