#
# Regular cron jobs for the xscopes-qt package
#
0 4	* * *	root	[ -x /usr/bin/xscopes-qt_maintenance ] && /usr/bin/xscopes-qt_maintenance
