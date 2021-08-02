#!/usr/bin/python3
import dnf
import dnf.cli.progress
import logging

myLogger = logging.Logger()
myLogger.setLevel(logging.info)
sh = logging.StreamHandler()
myLogger.addHandler(sh)


base = dnf.Base()
base.read_all_repos()
base.fill_sack()

rpms = ["/home/lijingwei/scap-security-guide-0.1.39-4.uel20.noarch.rpm"]
# 添加rpm包到base库中并获取package实例对象
package = base.add_remote_rpms(rpms)
# 安装这些包
for pkg in package:
    base.package_install(pkg)


print("deal dependency...")
progress = dnf.cli.progress.MultiFileProgressMeter()
# 处理依赖问题，安装包
base.download_packages(base.transaction.install_set, progress)
# 开始安装print("Installing...")
base.do_transaction()
