FROM alpine:latest

LABEL MAINTAINER="Santiago <sganis@gmail.com>" \
    Description="Small ssh docker container." \
    Version="1.0.0"

RUN apk --update add --no-cache openssh bash && \
	rm -rf /var/cache/apk/*

RUN echo "root:root" | chpasswd && \
	adduser support -s /bin/bash -D && \
	echo "support:support" | chpasswd 

RUN sed -ri s/#PermitRootLogin.*/PermitRootLogin\ yes/ /etc/ssh/sshd_config && \
	sed -ri 's/#Port 22/Port 22/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_key/HostKey \/etc\/ssh\/ssh_host_key/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_rsa_key/HostKey \/etc\/ssh\/ssh_host_rsa_key/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_dsa_key/HostKey \/etc\/ssh\/ssh_host_dsa_key/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_ecdsa_key/HostKey \/etc\/ssh\/ssh_host_ecdsa_key/g' /etc/ssh/sshd_config && \
	sed -ri 's/#HostKey \/etc\/ssh\/ssh_host_ed25519_key/HostKey \/etc\/ssh\/ssh_host_ed25519_key/g' /etc/ssh/sshd_config && \
	sed -ri 's/Subsystem.+/Subsystem sftp \/usr\/lib\/ssh\/sftp-server -l VERBOSE -u 002/g' /etc/ssh/sshd_config 

RUN /usr/bin/ssh-keygen -A
RUN ssh-keygen -t rsa -b 4096 -f /etc/ssh/ssh_host_key

EXPOSE 22
CMD ["/usr/sbin/sshd","-D"]



