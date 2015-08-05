/*
 * Copyright 2006-2015 Gentoo Foundation
 * Distributed under the terms of the GNU General Public License v2
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

bool verbose = false;

#define USE_PROCINFO

#if defined (USE_PROCINFO)

# if defined (__s390__) || defined (__s390x__)
#  define CPUINFO_FILE    "/proc/sysinfo"
#  define CPUINFO_FORMAT  "%64[^\t :]%*[ :]%256[^\n]%c"
# else
#  define CPUINFO_FILE    "/proc/cpuinfo"
#  define CPUINFO_FORMAT  "%64[^\t:]\t:%256[^\n]%c"
# endif

const char *procinfo_processor, *procinfo_platform;
const char *filename = CPUINFO_FILE;
#undef CPUINFO_FILE
#define CPUINFO_FILE filename

# define PROCINFO_PROCESSOR      0
# define PROCINFO_HARDWARE_PLATFORM 1

static void __eat_cpuinfo_space (char *buf)
{
	/* first eat trailing space */
	char *tmp = buf + strlen (buf) - 1;
	while (tmp > buf && isspace (*tmp))
		*tmp-- = '\0';
	/* then eat leading space */
	tmp = buf;
	while (*tmp && isspace (*tmp))
		tmp++;
	if (tmp != buf)
		memmove (buf, tmp, strlen (tmp) + 1);
	/* finally collapse whitespace */
	tmp = buf;
	while (tmp[0] && tmp[1]) {
		if (isspace (tmp[0]) && isspace (tmp[1])) {
			memmove (tmp, tmp + 1, strlen (tmp));
			continue;
		}
		++tmp;
	}
}

static int __linux_procinfo (int x, char *fstr, size_t s)
{
	FILE *fp;

	const char * const procinfo_keys[] = {
		/* --processor --hardware-platform */
		procinfo_processor, procinfo_platform,
	};

	if (verbose)
		printf ("### Looking for '%s':\n", procinfo_keys[x]);

	if ((fp = fopen (CPUINFO_FILE, "r")) != NULL) {
		char key[65], value[257], eol, *ret = NULL;

		while (fscanf (fp, CPUINFO_FORMAT, key, value, &eol) != EOF) {
			__eat_cpuinfo_space (key);
			if (verbose)
				printf ("### \t%s -> %s\n", key, value);
			if (!strcmp (key, procinfo_keys[x])) {
				__eat_cpuinfo_space (value);
				ret = value;
				break;
			}
			if (eol != '\n') {
				/* we need two fscanf's here in case the previous
				 * length limit caused us to read right up to the
				 * newline ... doing "%*[^\n]\n" wont eat the newline
				 */
				fscanf (fp, "%*[^\n]");
				fscanf (fp, "\n");
			}
		}
		fclose (fp);

		if (ret) {
			strncpy (fstr, ret, s);
			return 0;
		}
	}

	return -1;
}

#endif

static bool startswith(const char *s, const char *prefix)
{
	return strncmp(s, prefix, strlen(prefix)) == 0 ? true : false;
}

static const char *procinfo_guess_arch(const char *file)
{
	file = basename(filename);

	if (startswith(file, "blackfin"))
		return "bfin";

	if (startswith(file, "x86"))
		return "i386";

	if (startswith(file, "parisc"))
		return "hppa";

	if (startswith(file, "ppc64"))
		return "powerpc64";

	if (startswith(file, "ppc"))
		return "powerpc";

	return file;
}

static bool procinfo_init(void)
{
	static const struct {
		const char *arch;
		const char *processor;
		const char *platform;
	} procinfo_keys_all[] = {
		/* cpuname     --processor     --hardware-platform */
		{ "alpha",     "cpu model",    "system type" },
		{ "amd64",     "model name",   "vendor_id" },
		{ "arm",       "Processor",    "Hardware" },
		{ "avr32",     "processor",    "cpu family" },
		{ "bfin",      "CPU",          "BOARD Name" },
		{ "cris",      "cpu",          "cpu model" },
		{ "frv",       "CPU-Core",     "System" },
		{ "i386",      "model name",   "vendor_id" },
		{ "ia64",      "family",       "vendor" },
		{ "hppa",      "cpu",          "model" },
		{ "m68k",      "CPU",          "MMU" },
		{ "microblaze","CPU-Ver",      "FPGA-Arch" },
		{ "mips",      "cpu model",    "system type" },
		{ "powerpc",   "cpu",          "machine" },
		{ "s390",      "Type",         "Manufacturer" },
		{ "sh",        "cpu type",     "machine" },
		{ "sparc",     "type",         "cpu" },
		{ "vax",       "cpu type",     "cpu" },
	};

	size_t i;
	const char *arch = procinfo_guess_arch(filename);
	for (i = 0; i < ARRAY_SIZE(procinfo_keys_all); ++i)
		if (startswith(arch, procinfo_keys_all[i].arch)) {
			procinfo_processor = procinfo_keys_all[i].processor;
			procinfo_platform = procinfo_keys_all[i].platform;
			return true;
		}

	warnx("could not detect arch with %s", filename);
	return false;
}

static bool process_file(const char *file)
{
	static char processor[257];
	static char hardware_platform[257];
	bool ret = true;

	filename = file;
	printf(">>> Parsing data out of %s\n", filename);
	if (!procinfo_init())
		return false;

	if (0 > __linux_procinfo (PROCINFO_PROCESSOR, processor, sizeof processor)) {
		strcpy(processor, "failed");
		printf("!!!");
		ret = false;
	} else
		printf(">>>");
	printf(" processor         = %s\n", processor);

	if (0 > __linux_procinfo (PROCINFO_HARDWARE_PLATFORM, hardware_platform, sizeof hardware_platform)) {
		strcpy(hardware_platform, "failed");
		printf("!!!");
		ret = false;
	} else
		printf(">>>");
	printf(" hardware_platform = %s\n", hardware_platform);

	printf("\n");

	return ret;
}

static bool process_dir(const char *path)
{
	bool ret = true;
	char *sub_path;

	DIR *dir = opendir(path);
	if (!dir)
		err(1, "could not opendir %s", path);

	struct dirent *de;
	while ((de = readdir(dir)) != NULL) {
		if (de->d_name[0] == '.')
			continue;

		asprintf(&sub_path, "%s/%s", path, de->d_name);
		switch (de->d_type) {
		case DT_REG:
			if (!process_file(sub_path))
				ret = false;
			break;
		case DT_DIR:
			if (!process_dir(sub_path))
				ret = false;
			break;
		}
		free(sub_path);
	}

	closedir(dir);

	return ret;
}

static bool process_one(const char *path)
{
	struct stat st;

	if (stat(path, &st)) {
		warn("could not stat %s", path);
		return false;
	}

	if (S_ISREG(st.st_mode))
		return process_file(path);
	else if (S_ISDIR(st.st_mode))
		return process_dir(path);
	else
		return false;
}

int main(int argc, char *argv[])
{
	int i, ret;

	while ((i = getopt(argc, argv, "v")) != -1) {
		switch (i) {
		case 'v':
			verbose = true;
			break;
		default:
 usage:
			errx(1, "Usage: uname-test [-v] <file|dir> [more paths]");
		}
	}
	if (optind == argc)
		goto usage;

	ret = 0;
	for (i = optind; i < argc; ++i)
		if (!process_one(argv[i]))
			ret = 1;

	return ret;
}
