
define Profile/Anvil
  NAME:=Anvil
  PACKAGES:= -dnsmasq -iptables -ppp -ppp-mod-pppoe -kmod-ipt-nathelper -firewall -cwmpd
endef

define Profile/Anvil/Description
	Anvil Profile
endef
$(eval $(call Profile,Anvil))
