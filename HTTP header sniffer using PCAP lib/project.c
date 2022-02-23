#include <pcap.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <ctype.h>

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6
#define SIZE_ETHERNET 14

	/* Ethernet header */
	struct sniff_ethernet {
		u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
		u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
        u_short ether_type; /* IP? ARP? RARP? etc */
	};

	/* IP header */
	struct sniff_ip {
		u_char ip_vhl;		/* version << 4 | header length >> 2 */
		u_char ip_tos;		/* type of service */
		u_short ip_len;		/* total length */
		u_short ip_id;		/* identification */
		u_short ip_off;		/* fragment offset field */
	#define IP_RF 0x8000		/* reserved fragment flag */
	#define IP_DF 0x4000		/* don't fragment flag */
	#define IP_MF 0x2000		/* more fragments flag */
	#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
		u_char ip_ttl;		/* time to live */
		u_char ip_p;		/* protocol */
		u_short ip_sum;		/* checksum */
		struct in_addr ip_src,ip_dst; /* source and dest address */
	};
	#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
	#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

	/* TCP header */
	typedef u_int tcp_seq;

	struct sniff_tcp {
		u_short th_sport;	/* source port */
		u_short th_dport;	/* destination port */
		tcp_seq th_seq;		/* sequence number */
		tcp_seq th_ack;		/* acknowledgement number */
		u_char th_offx2;	/* data offset, rsvd */
	#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
		u_char th_flags;
	#define TH_FIN 0x01
	#define TH_SYN 0x02
	#define TH_RST 0x04
	#define TH_PUSH 0x08
	#define TH_ACK 0x10
	#define TH_URG 0x20
	#define TH_ECE 0x40
	#define TH_CWR 0x80
	#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
		u_short th_win;		/* window */
		u_short th_sum;		/* checksum */
		u_short th_urp;		/* urgent pointer */
    };

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet); 


int main(int argc, char *argv[])
{

    pcap_if_t *alldevsp;            // device list pointer
    pcap_if_t *d;                   //device pointer
    pcap_t *handle;                 //session handler

    int tmp =0, i=0;
	char errbuf[PCAP_ERRBUF_SIZE];	// Error string 
	
    struct bpf_program fp;		    // The compiled filter expression 
	char filter_exp[] = "port 80";	// The filter expression set to HTTP
	
    bpf_u_int32 maskp;	        	// The netmask of the sniffing device 
	bpf_u_int32 netp;		        // The IP of the sniffing device


    /* find the devices */
    printf("-----------list of network devices-----------\n");
    tmp = pcap_findalldevs(&alldevsp, errbuf);
    if(tmp==-1)
    {
        printf("**pcap_findalldevs error**: %s\n", errbuf);
        return -1;
    }
    /* print the list of devices */
    for(d=alldevsp; d; d=d->next){
        printf("%d. %s", i++, d->name);
         if(d->description){
            printf("(%s)\n", d->description);
        } else{
            printf("(None)\n");
        }
    }
    /* let user select the device */
    printf("Select the number of device you want to sniff > ");
    scanf("%d", &i);

    for(d=alldevsp;i>0;d=d->next, i--);
    printf("selected device : %s \n", d->name);

    /* Find the IPv4 network number and netmask for the device */
    tmp = pcap_lookupnet(d->name, &netp, &maskp, errbuf);
    if(tmp == -1){
        printf("**pcap_lookupnet error** : %s\n", errbuf);
        return -1;
    }

    /* created the session in promiscuous mode, buffer size 8192, 1000ms time out */
    handle = pcap_open_live(d->name, BUFSIZ, 1, 1000, errbuf);
    if(handle == NULL) {
        fprintf(stderr, "**pcap_open_live error** %s : %s\n", d->name, errbuf);
        return -1;
    }

    /* Compile and apply the filter to HTTP(port 80) */
    if(pcap_compile(handle, &fp, filter_exp, 0, netp) == -1) {
        fprintf(stderr, "**pcap_compile error** %s: %s\n", filter_exp, pcap_geterr(handle));
        return -1;
    }
    if(pcap_setfilter(handle, &fp) == -1){
        fprintf(stderr, "**pcap_setfilter error** %s: %s\n", filter_exp, pcap_geterr(handle));
        return -1;
    }


    /* start sniffing, until ending condition occurs */
    pcap_loop(handle, 0, callback, NULL);

    /* clean up */
    pcap_freecode(&fp);
    pcap_close(handle);

    return 0;


}

//callback function for the sniffing event
void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){

    static int count = 1;                   // packet counter 

	/* pointers to packet headers */
	const struct sniff_ethernet *ethernet;  // The ethernet header [1] 
	const struct sniff_ip *ip;              // The IP header 
	const struct sniff_tcp *tcp;            // The TCP header 
	const char *payload;                    // Packet payload 

	int size_ip;
	int size_tcp;
	int size_payload;

    int i;
    const u_char *ch;
    
    
	/* define Ethernet header */
	ethernet = (struct sniff_ethernet*)(packet);

	/* define/compute IP header offset */
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}

	/* define/compute TCP header offset */
	tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
	size_tcp = TH_OFF(tcp)*4;
	if (size_tcp < 20) {
		printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
		return;
	}

	/* define/compute TCP Payload (segment) offset */
	payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);

	/* compute TCP Payload (segment) size */
	size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);

    /* analyze payload data */
    ch = payload;

    /*check the HTTP Method */
    char *METHOD;
    if(*ch == 'H' && *++ch == 'T'){
        METHOD = "RESPONSE";
    }
    else{
        METHOD = "REQUEST";
    }

    ch = payload;

    /* print the HTTP header*/
	if (size_payload > 0) {
		/* print Packet Number, IP, PORT, Method  info*/
        printf("%d %s:%d %s:%d HTTP %s \n", count, inet_ntoa(ip->ip_src), ntohs(tcp->th_sport), inet_ntoa(ip->ip_dst), ntohs(tcp->th_dport), METHOD);

        count++;           // increase packet count

        /* print request line & header line*/
        if(*ch=='}'){}     //'}' bug fix
        else{
        
            for(i = 0; i < size_payload; i++) {
            if (isprint(*ch)){
                printf("%c", *ch);
            }
            else if(*ch=='\n'){
                printf("%c",*ch);

                if(*++ch=='\r'){  //check if it is the end of header, which is '/r/n/r/n'
                    printf("\n");
                    break;
                } else{
                    ch--;
                }

            }
            ch++;
            }
    	}
    }
    
}
    
