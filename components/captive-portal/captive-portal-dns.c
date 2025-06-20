/**captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.

  Contains some modified example code from here:
  https://github.com/cornelis-61/esp32_Captdns/blob/master/main/captdns.c

  Original Example Code Header:
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
* this notice you can do whatever you want with this stuff. If we meet some day,
* and you think this stuff is worth it, you can buy me a beer in return.
*
* modified for ESP32 by Cornelis
*
* ----------------------------------------------------------------------------
*/
#include <sys/time.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "esp_netif.h"
#include "captive-portal-dns.h"

static const char *DNS_TAG = "cap-dns";

static const char *dns_uri = CAPTIVE_PORTAL_DNS_URI;
static const char *dns_ns = CAPTIVE_PORTAL_DNS_NS;

static int sock_fd;

// Function to put unaligned 16-bit network values
static void setn16(void *pp, int16_t n)
{
  char *p = pp;
  *p++ = (n >> 8);
  *p++ = (n & 0xff);
}

// Function to put unaligned 32-bit network values
static void setn32(void *pp, int32_t n)
{
  char *p = pp;
  *p++ = (n >> 24) & 0xff;
  *p++ = (n >> 16) & 0xff;
  *p++ = (n >> 8) & 0xff;
  *p++ = (n & 0xff);
}

/**
 * @brief Converts a 16-bit value from network byte order to host byte order.
 *
 * Takes a 16-bit value in network byte order (big-endian) and converts it to host byte order.
 * This means 0xXXYY in network byte order becomes 0xYYXX in host byte order on little-endian systems.
 *
 * @param in Pointer to the 16-bit value in network byte order.
 * @return The 16-bit value in host byte order.
 */
static uint16_t my_ntohs(const uint16_t *in)
{
  const char *p = (const char *)in;
  return ((p[0] << 8) & 0xff00) | (p[1] & 0xff);
}

// Parses a label into a C-string containing a dotted
// Returns pointer to start of next fields in packet
static char *label_to_str(char *packet, char *labelPtr, int packetSz, char *res, int resMaxLen)
{
  int i, j, k;
  char *endPtr = NULL;
  i = 0;
  do
  {
    if ((*labelPtr & 0xC0) == 0)
    {
      j = *labelPtr++; // skip past length
      // Add separator period if there already is data in res
      if (i < resMaxLen && i != 0)
        res[i++] = '.';
      // Copy label to res
      for (k = 0; k < j; k++)
      {
        if ((labelPtr - packet) > packetSz)
          return NULL;
        if (i < resMaxLen)
          res[i++] = *labelPtr++;
      }
    }
    else if ((*labelPtr & 0xC0) == 0xC0)
    {
      // Compressed label pointer
      endPtr = labelPtr + 2;
      int offset = my_ntohs(((uint16_t *)labelPtr)) & 0x3FFF;
      // Check if offset points to somewhere outside of the packet
      if (offset > packetSz)
        return NULL;
      labelPtr = &packet[offset];
    }
    // check for out-of-bound-ness
    if ((labelPtr - packet) > packetSz)
      return NULL;
  } while (*labelPtr != 0);
  res[i] = 0; // zero-terminate
  if (endPtr == NULL)
    endPtr = labelPtr + 1;
  return endPtr;
}

// Converts a dotted hostname to the weird label form dns uses.
static char *str_to_label(char *str, char *label, int max_len)
{
  char *len = label;   // ptr to len byte
  char *p = label + 1; // ptr to next label byte to be written
  while (1)
  {
    if (*str == '.' || *str == 0)
    {
      *len = ((p - len) - 1); // write len of label bit
      len = p;                // pos of len for next part
      p++;                    // data ptr is one past len
      if (*str == 0)
        break; // done
      str++;
    }
    else
    {
      *p++ = *str++; // copy byte
    }
    // Check if we are out of bounds
    if ((p - label) >= max_len)
    {
      ESP_LOGE(DNS_TAG, "Not enough space in DNS label buffer");
      return NULL; // not enough space
    }
  }
  *len = 0;
  return p; // ptr to first free byte in resp
}

static char *add_resource_footer(char *p, int type, int cl, int ttl, int rdlength)
{
  DnsResourceFooter *rf = (DnsResourceFooter *)p;
  setn16(&rf->type, type);
  setn16(&rf->cl, cl);
  setn32(&rf->ttl, ttl);
  setn16(&rf->rdlength, rdlength);
  return p + sizeof(DnsResourceFooter);
}

// Receive a DNS packet and maybe send a response back
static void dns_recv(struct sockaddr_in *premote_addr, char *pusrdata, unsigned short length)
{

  char buff[WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN];
  char reply[WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN];
  int i;
  char *rend = &reply[length]; // Pointer to the end of the reply buffer, where we will write the response
  char *p = pusrdata;
  DnsHeader *hdr = (DnsHeader *)p;
  DnsHeader *rhdr = (DnsHeader *)&reply[0];
  p += sizeof(DnsHeader);

  // Some sanity checks:
  if (length > WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN)
    return; // Packet is longer than DNS implementation allows
  if (length < sizeof(DnsHeader))
    return; // Packet is too short
  if (hdr->ancount || hdr->nscount || hdr->arcount)
    return; // this is a reply, don't know what to do with it
  if (hdr->flags & WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_TC)
    return; // truncated, can't use this
  // Reply is basically the request plus the needed data
  memcpy(reply, pusrdata, length);
  rhdr->flags |= WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_QR;

  for (i = 0; i < my_ntohs(&hdr->qdcount); i++)
  {
    // Grab the labels in the q string
    p = label_to_str(pusrdata, p, length, buff, sizeof(buff));
    if (p == NULL)
      return;
    DnsQuestionFooter *qf = (DnsQuestionFooter *)p;
    p += sizeof(DnsQuestionFooter);

    ESP_LOGI(DNS_TAG, "DNS: WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_Q (type 0x%X cl 0x%X) for %s\n", my_ntohs(&qf->type), my_ntohs(&qf->cl), buff);

    if (my_ntohs(&qf->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_A)
    {
      // IPv4 address request. We will reply with our own IP.
      // Add the label
      rend = str_to_label(buff, rend, sizeof(reply) - (rend - reply));
      if (rend == NULL)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS label");
        return;
      }
      if (rend + sizeof(DnsResourceFooter) + 4 - reply > WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS A response");
        return;
      }

      rend = add_resource_footer(rend, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_A, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_IN, 0, 4);

      // Grab the current IP of the softap interface
      esp_netif_ip_info_t info;
      esp_netif_get_ip_info(esp_netif_next(NULL), &info);
      *rend++ = ip4_addr1(&info.ip);
      *rend++ = ip4_addr2(&info.ip);
      *rend++ = ip4_addr3(&info.ip);
      *rend++ = ip4_addr4(&info.ip);
      setn16(&rhdr->ancount, my_ntohs(&rhdr->ancount) + 1);
    }
    else if (my_ntohs(&qf->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_NS)
    {
      // Request for nameserver. We will reply with brief nonsensensical nameserver. It will be resolved (see above) to our IP.
      rend = str_to_label(buff, rend, sizeof(reply) - (rend - reply)); // Add the label
      if (rend == NULL)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS label");
        return;
      }
      const size_t dns_ns_len = strlen(dns_ns) + 2; // +2 for the length byte and the null terminator
      if (rend + sizeof(DnsResourceFooter) + dns_ns_len - reply > WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS NS response");
        return;
      }

      rend = add_resource_footer(rend, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_NS, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_IN, 0, dns_ns_len);
      // Add the nameserver string
      *rend++ = strlen(dns_ns); // Length of the nameserver string
      strcpy(rend, dns_ns);     // Copy the nameserver string
      rend += strlen(dns_ns) + 1;
      setn16(&rhdr->ancount, my_ntohs(&rhdr->ancount) + 1);
    }
    else if (my_ntohs(&qf->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_URI)
    {
      //
      rend = str_to_label(buff, rend, sizeof(reply) - (rend - reply)); // Add the label
      if (rend == NULL)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS label");
        return;
      }
      if (rend + sizeof(DnsResourceFooter) + sizeof(DnsUriHdr) + strlen(dns_uri) - reply > WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN)
      {
        ESP_LOGE(DNS_TAG, "Not enough space in reply buffer for DNS URI response");
        return;
      }

      // No device I have has tested this. I wonder if the rdlength is correct.
      // Is cleaned up, but same as the original. It is curious, that here we do not copy a null-terminator and dont move rend past it, while we do in the NS case.
      rend = add_resource_footer(rend, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_URI, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_URI, 0, sizeof(DnsUriHdr) + strlen(dns_uri));

      // Add the URI header
      DnsUriHdr *uh = (DnsUriHdr *)rend;
      setn16(&uh->prio, 10);
      setn16(&uh->weight, 1);
      rend += sizeof(DnsUriHdr);

      // Add the URI string
      memcpy(rend, dns_uri, strlen(dns_uri));
      rend += strlen(dns_uri);
      setn16(&rhdr->ancount, my_ntohs(&rhdr->ancount) + 1);
    }
  }
  // Send the response
  sendto(sock_fd, (uint8_t *)reply, rend - reply, 0, (struct sockaddr *)premote_addr, sizeof(struct sockaddr_in));
}

static void dns_task(void *pvParameters)
{
  struct sockaddr_in server_addr;
  uint32_t ret;
  struct sockaddr_in from;
  socklen_t fromlen;
  char udp_msg[WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN];

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(53);
  server_addr.sin_len = sizeof(server_addr);

  // Create a UDP socket
  do
  {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
      ESP_LOGI(DNS_TAG, "dns_task failed to create sock!");
      vTaskDelay(1000 / portTICK_RATE_MS);
    }
  } while (sock_fd == -1);

  // Bind the socket to the address and port
  do
  {
    ret = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret != 0)
    {
      ESP_LOGI(DNS_TAG, "dns_task failed to bind sock!");
      vTaskDelay(1000 / portTICK_RATE_MS);
    }
  } while (ret != 0);
  ESP_LOGI(DNS_TAG, "DNS initialized.");

  // Run the DNS server loop
  while (1)
  {
    memset(&from, 0, sizeof(from));
    fromlen = sizeof(struct sockaddr_in);
    ret = recvfrom(sock_fd, (uint8_t *)udp_msg, WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN, 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);
    if (ret > 0)
    {
      dns_recv(&from, udp_msg, ret);
    }
  }
  ESP_LOGE(DNS_TAG, "dns_task exiting, this should never happen!");
  close(sock_fd);
  vTaskDelete(NULL);
}

void wifi_captive_portal_esp_idf_dns_init(void)
{
  xTaskCreate(dns_task, (const char *)"dns_task", 10000, NULL, 3, NULL);
}
