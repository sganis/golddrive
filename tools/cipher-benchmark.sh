#!/bin/bash

# Based on: http://www.systutorials.com/5450/improving-sshscp-performance-by-choosing-ciphers/#comment-28725
#
# You should set up PublicKey authentication so that you don't have to type your
# password for every cipher tested.

set -o pipefail

ciphers="$@"
if [[ -n "$ciphers" ]]; then echo "User-supplied ciphers: $ciphers"; fi

if [[ -z "$ciphers" ]]; then
  ciphers=$(egrep '^\s*Ciphers' /etc/ssh/sshd_config|sed 's/Ciphers//; s/,/ /')
  if [[ -n "$ciphers" ]]; then echo "/etc/ssh/sshd_config allows these ciphers: $ciphers"; fi
fi

if [[ -z "$ciphers" ]]; then
  ciphers=$(echo $(ssh -Q cipher))
  if [[ -n "$ciphers" ]]; then echo "ssh -Q cipher reports these ciphers: $ciphers"; fi
fi

if [[ -z "$ciphers" ]]; then
  read -rd '' ciphers <<EOF
3des-cbc aes128-cbc aes128-ctr aes128-gcm@openssh.com aes192-cbc aes192-ctr
aes256-cbc aes256-ctr aes256-gcm@openssh.com arcfour arcfour128 arcfour256
blowfish-cbc cast128-cbc chacha20-poly1305@openssh.com rijndael-cbc@lysator.liu.se
EOF
  echo "Default cipher test list: $ciphers"
fi

echo
echo "For each cipher, will transfer 1000 MB of zeros to/from localhost."
echo

tmp=$(mktemp)
for i in $ciphers
do
  echo -n "$i: "
  dd if=/dev/zero bs=1000000 count=1000 2> /dev/null |
  ssh -c $i -o Compression=no localhost "(time -p cat) > /dev/null" > $tmp 2>&1
  
  if [[ $? == 0 ]]; then
      grep real $tmp | awk '{print 1000 / $2" MB/s" }'
  else
      echo "failed, for why run: ssh -vc $i localhost"
  fi
done
