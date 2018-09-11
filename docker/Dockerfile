FROM alpine:latest
LABEL MAINTAINER="Santiago <sganis@gmail.com>" \
    Description="Ssh and Samba docker container." \
    Version="1.0.0"

RUN apk --update add --no-cache openssh bash \
	samba samba-common-tools supervisor && \
	rm -rf /var/cache/apk/*

RUN echo "root:root" | chpasswd && \
	adduser support -s /bin/bash -D && \
	echo "support:support" | chpasswd 

RUN sed -i s/#PermitRootLogin.*/PermitRootLogin\ yes/ /etc/ssh/sshd_config && \
	sed -ie 's/#Port 22/Port 22/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_key/HostKey \/etc\/ssh\/ssh_host_key/g' /etc/ssh/sshd_config && \
	sed -ir 's/#HostKey \/etc\/ssh\/ssh_host_rsa_key/HostKey \/etc\/ssh\/ssh_host_rsa_key/g' /etc/ssh/sshd_config && \
	sed -ir 's/#HostKey \/etc\/ssh\/ssh_host_dsa_key/HostKey \/etc\/ssh\/ssh_host_dsa_key/g' /etc/ssh/sshd_config && \
	sed -ir 's/#HostKey \/etc\/ssh\/ssh_host_ecdsa_key/HostKey \/etc\/ssh\/ssh_host_ecdsa_key/g' /etc/ssh/sshd_config && \
	sed -ir 's/#HostKey \/etc\/ssh\/ssh_host_ed25519_key/HostKey \/etc\/ssh\/ssh_host_ed25519_key/g' /etc/ssh/sshd_config 

RUN /usr/bin/ssh-keygen -A
RUN ssh-keygen -t rsa -b 4096 -f /etc/ssh/ssh_host_key

EXPOSE 22

CMD ["/usr/sbin/sshd","-D"]
