proxmox-mini-journalreader(1)

# NAME
proxmox-mini-journalreader - read systemd journald logs

# SYNOPSIS
*proxmox-mini-journalreader* [OPTIONS]

# DESCRIPTION
Provides access to systemd journald logs, allowing users to fetch log entries
based on timestamps, cursors, or a specified number of last entries.

# OPTIONS
*-b* <timestamp>
	Begin at this UNIX epoch timestamp.

*-e* <timestamp>
	End at this UNIX epoch timestamp.

*-d* <directory>
	Specify a custom journal directory.

*-n* <integer>
	Print the last number of entries logged.

*-f* <cursor>
	Print starting from this cursor.

*-t* <cursor>
	Print until (to) this cursor.

*-h*
	Display the help message.

# NOTES
. No range option means dump all entries.
. Range conflicts with -n.
. -b and -f conflict.
. -e and -t conflict.

# SEE ALSO
journalctl(1), systemd-journald.service(8)

# AUTHORS
The Proxmox Support Team <support@proxmox.com>.
