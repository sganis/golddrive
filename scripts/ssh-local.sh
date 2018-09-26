#!/bin/sh
## Build openssh
## Build openssl 1.0.1p 
# configure && make && sudo make install (will install in /usr/local/ssh)
## Build openssh hpn
# autoreconf
# configure --with-ssh-dir=/usr/local/ssh
# make -j8
# sudo make install (will install in /usr/local/sbin)
## Create an unpriviledged user in ubuntu
# sudo mkdir /var/empty
# sudo chown root:sys /var/empty
# sudo chmod 755 /var/empty
# sudo groupadd sshd
# sudo useradd -g sshd -c 'sshd privsep' -d /var/empty -s /bin/false sshd

## copy next script to /etc/init.d/ssh-local

### BEGIN INIT INFO
# Provides:     sshd-local
# Required-Start:   $remote_fs $syslog
# Required-Stop:    $remote_fs $syslog
# Default-Start:    2 3 4 5
# Default-Stop:     
# Short-Description:    Local OpenBSD Secure Shell server
### END INIT INFO

set -e

# /etc/init.d/ssh-local: start and stop the local OpenBSD secure shell daemon

test -x /usr/local/sbin/sshd || exit 0
( /usr/local/sbin/sshd -\? 2>&1 | grep -q OpenSSH ) 2>/dev/null || exit 0

umask 022

if test -f /etc/default/ssh; then
    . /etc/default/ssh
fi

if test -f /lib/lsb/init-functions; then
    . /lib/lsb/init-functions
fi

if [ -n "$2" ]; then
    SSHD_OPTS="$SSHD_OPTS -f /usr/local/etc/sshd_config $2"
fi

# Are we running from init?
run_by_init() {
    ([ "$previous" ] && [ "$runlevel" ]) || [ "$runlevel" = S ]
}

check_for_no_start() {
    # forget it if we're trying to start, and /etc/ssh/sshd_not_to_be_run exists
    if [ -e /etc/ssh/sshd_not_to_be_run ]; then 
    if [ "$1" = log_end_msg ]; then
        log_end_msg 0 || true
    fi
    if ! run_by_init; then
        log_action_msg "Local OpenBSD Secure Shell server not in use (/etc/ssh/sshd_not_to_be_run)" || true
    fi
    exit 0
    fi
}

check_dev_null() {
    if [ ! -c /dev/null ]; then
    if [ "$1" = log_end_msg ]; then
        log_end_msg 1 || true
    fi
    if ! run_by_init; then
        log_action_msg "/dev/null is not a character device!" || true
    fi
    exit 1
    fi
}

check_privsep_dir() {
    # Create the PrivSep empty dir if necessary
    if [ ! -d /var/run/sshd-local ]; then
    mkdir /var/run/sshd-local
    chmod 0755 /var/run/sshd-local
    fi
}

check_config() {
    if [ ! -e /etc/ssh/sshd_not_to_be_run ]; then
    /usr/local/sbin/sshd $SSHD_OPTS -t || exit 1
    fi
}

export PATH="${PATH:+$PATH:}/usr/local/sbin:/usr/sbin:/sbin"

case "$1" in
  start)
    check_privsep_dir
    check_for_no_start
    check_dev_null
    log_daemon_msg "Starting local OpenBSD Secure Shell server" "sshd" || true
    if start-stop-daemon --start --quiet --oknodo --pidfile /var/run/sshd-local.pid --exec /usr/local/sbin/sshd -- $SSHD_OPTS; then
        log_end_msg 0 || true
    else
        log_end_msg 1 || true
    fi
    ;;
  stop)
    log_daemon_msg "Stopping local OpenBSD Secure Shell server" "sshd" || true
    if start-stop-daemon --stop --quiet --oknodo --pidfile /var/run/sshd-local.pid; then
        log_end_msg 0 || true
    else
        log_end_msg 1 || true
    fi
    ;;

  reload|force-reload)
    check_for_no_start
    check_config
    log_daemon_msg "Reloading local OpenBSD Secure Shell server's configuration" "sshd" || true
    if start-stop-daemon --stop --signal 1 --quiet --oknodo --pidfile /var/run/sshd-local.pid --exec /usr/local/sbin/sshd; then
        log_end_msg 0 || true
    else
        log_end_msg 1 || true
    fi
    ;;

  restart)
    check_privsep_dir
    check_config
    log_daemon_msg "Restarting local OpenBSD Secure Shell server" "sshd" || true
    start-stop-daemon --stop --quiet --oknodo --retry 30 --pidfile /var/run/sshd-local.pid
    check_for_no_start log_end_msg
    check_dev_null log_end_msg
    if start-stop-daemon --start --quiet --oknodo --pidfile /var/run/sshd-local.pid --exec /usr/local/sbin/sshd -- $SSHD_OPTS; then
        log_end_msg 0 || true
    else
        log_end_msg 1 || true
    fi
    ;;

  try-restart)
    check_privsep_dir
    check_config
    log_daemon_msg "Restarting local OpenBSD Secure Shell server" "sshd" || true
    RET=0
    start-stop-daemon --stop --quiet --retry 30 --pidfile /var/run/sshd-local.pid || RET="$?"
    case $RET in
        0)
        # old daemon stopped
        check_for_no_start log_end_msg
        check_dev_null log_end_msg
        if start-stop-daemon --start --quiet --oknodo --pidfile /var/run/sshd-local.pid --exec /usr/local/sbin/sshd -- $SSHD_OPTS; then
            log_end_msg 0 || true
        else
            log_end_msg 1 || true
        fi
        ;;
        1)
        # daemon not running
        log_progress_msg "(not running)" || true
        log_end_msg 0 || true
        ;;
        *)
        # failed to stop
        log_progress_msg "(failed to stop)" || true
        log_end_msg 1 || true
        ;;
    esac
    ;;

  status)
    status_of_proc -p /var/run/sshd-local.pid /usr/local/sbin/sshd sshd && exit 0 || exit $?
    ;;

  *)
    log_action_msg "Usage: /etc/init.d/ssh-local {start|stop|reload|force-reload|restart|try-restart|status}" || true
    exit 1
esac

exit 0