/*
   Unix SMB/CIFS implementation.
   test suite for spoolss rpc operations

   Copyright (C) Guenther Deschner 2009-2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************
****************************************************************************/

#include "spoolss.h"
#include "string.h"
#include "torture.h"

/****************************************************************************
****************************************************************************/

static BOOL test_OpenPrinter(struct torture_context *tctx,
			     LPSTR printername,
			     LPPRINTER_DEFAULTS defaults,
			     LPHANDLE handle)
{
	torture_comment(tctx, "Testing OpenPrinter(%s)", printername);

	if (!OpenPrinter(printername, handle, defaults)) {
		char tmp[1024];
		sprintf(tmp, "failed to open printer %s, error was: 0x%08x\n",
			printername, GetLastError());
		torture_fail(tctx, tmp);
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_ClosePrinter(struct torture_context *tctx,
			      HANDLE handle)
{
	torture_comment(tctx, "Testing ClosePrinter");

	if (!ClosePrinter(handle)) {
		char tmp[1024];
		sprintf(tmp, "failed to close printer, error was: %s\n",
			errstr(GetLastError()));
		torture_fail(tctx, tmp);
	}

	return TRUE;
}


/****************************************************************************
****************************************************************************/

static BOOL test_EnumPrinters(struct torture_context *tctx,
			      LPSTR servername)
{
	DWORD levels[]  = { 1, 2, 5 };
	DWORD success[] = { 1, 1, 1 };
	DWORD i;
	DWORD flags = PRINTER_ENUM_NAME;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumPrinters level %d", levels[i]);

		EnumPrinters(flags, servername, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumPrinters(flags, servername, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumPrinters failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		if (tctx->print) {
			print_printer_info_bylevel(levels[i], buffer, returned);
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumDrivers(struct torture_context *tctx,
			     LPSTR servername,
			     LPSTR architecture)
{
	DWORD levels[]  = { 1, 2, 3, 4, 5, 6 };
	DWORD success[] = { 1, 1, 1, 1, 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumPrinterDrivers level %d", levels[i]);

		EnumPrinterDrivers(servername, architecture, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumPrinterDrivers(servername, architecture, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumPrinterDrivers failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		if (tctx->print) {
			print_driver_info_bylevel(levels[i], buffer, returned);
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetForm(struct torture_context *tctx,
			 LPSTR servername,
			 HANDLE handle,
			 LPSTR formname)
{
	DWORD levels[]  = { 1, 2 };
	DWORD success[] = { 1, 0 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing GetForm(%s) level %d", formname, levels[i]);

		GetForm(handle, formname, levels[i], NULL, 0, &needed);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!GetForm(handle, formname, levels[i], buffer, needed, &needed)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "GetForm failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumForms(struct torture_context *tctx,
			   LPSTR servername,
			   HANDLE handle)
{
	DWORD levels[]  = { 1, 2 };
	DWORD success[] = { 1, 0 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumForms level %d", levels[i]);

		EnumForms(handle, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumForms(handle, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumForms failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumPorts(struct torture_context *tctx,
			   LPSTR servername)
{
	DWORD levels[]  = { 1, 2 };
	DWORD success[] = { 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumPorts level %d", levels[i]);

		EnumPorts(servername, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumPorts(servername, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumPorts failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumMonitors(struct torture_context *tctx,
			      LPSTR servername)
{
	DWORD levels[]  = { 1, 2 };
	DWORD success[] = { 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumMonitors level %d", levels[i]);

		EnumMonitors(servername, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumMonitors(servername, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumMonitors failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumPrintProcessors(struct torture_context *tctx,
				     LPSTR servername,
				     LPSTR architecture)
{
	DWORD levels[]  = { 1 };
	DWORD success[] = { 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumPrintProcessors level %d", levels[i]);

		EnumPrintProcessors(servername, architecture, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumPrintProcessors(servername, architecture, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumPrintProcessors failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumPrintProcessorDatatypes(struct torture_context *tctx,
					     LPSTR servername)
{
	DWORD levels[]  = { 1 };
	DWORD success[] = { 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumPrintProcessorDatatypes level %d", levels[i]);

		EnumPrintProcessorDatatypes(servername, "winprint", levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumPrintProcessorDatatypes(servername, "winprint", levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumPrintProcessorDatatypes failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumPrinterKey(struct torture_context *tctx,
				LPSTR servername,
				HANDLE handle,
				LPCSTR key)
{
	LPSTR buffer = NULL;
	DWORD needed = 0;
	DWORD err = 0;
	char tmp[1024];

	torture_comment(tctx, "Testing EnumPrinterKey(%s)", key);

	err = EnumPrinterKey(handle, key, NULL, 0, &needed);
	if (err == ERROR_MORE_DATA) {
		buffer = (LPTSTR)malloc(needed);
		torture_assert(tctx, buffer, "malloc failed");
		err = EnumPrinterKey(handle, key, buffer, needed, &needed);
	}
	if (err) {
		sprintf(tmp, "EnumPrinterKey(%s) failed on [%s] (buffer size = %d), error: %s\n",
			key, servername, needed, errstr(err));
		torture_fail(tctx, tmp);
	}

	if (tctx->print) {
		print_printer_keys(buffer);
	}

	free(buffer);

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrinter(struct torture_context *tctx,
			    LPSTR printername,
			    HANDLE handle)
{
	DWORD levels[]  = { 1, 2, 3, 4, 5, 6, 7, 8 };
	DWORD success[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing GetPrinter level %d", levels[i]);

		GetPrinter(handle, levels[i], NULL, 0, &needed);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!GetPrinter(handle, levels[i], buffer, needed, &needed)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "GetPrinter failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], printername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		if (tctx->print) {
			print_printer_info_bylevel(levels[i], buffer, 1);
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrinterDriver(struct torture_context *tctx,
				  LPSTR printername,
				  LPSTR architecture,
				  HANDLE handle)
{
	DWORD levels[]  = { 1, 2, 3, 4, 5, 6, 8 };
	DWORD success[] = { 1, 1, 1, 1, 1, 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing GetPrinterDriver level %d", levels[i]);

		GetPrinterDriver(handle, architecture, levels[i], NULL, 0, &needed);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!GetPrinterDriver(handle, architecture, levels[i], buffer, needed, &needed)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "GetPrinterDriver failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], printername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		if (tctx->print) {
			print_driver_info_bylevel(levels[i], buffer, 1);
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}


/****************************************************************************
****************************************************************************/

static BOOL test_EnumJobs(struct torture_context *tctx,
			  LPSTR printername,
			  HANDLE handle)
{
	DWORD levels[]  = { 1, 2, 3, 4 };
	DWORD success[] = { 1, 1, 1, 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD returned = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing EnumJobs level %d", levels[i]);

		EnumJobs(handle, 0, 100, levels[i], NULL, 0, &needed, &returned);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!EnumJobs(handle, 0, 100, levels[i], buffer, needed, &needed, &returned)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "EnumJobs failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], printername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EnumPrinterDataEx(struct torture_context *tctx,
				   LPSTR servername,
				   LPSTR keyname,
				   HANDLE handle,
				   LPBYTE *buffer_p,
				   DWORD *returned_p)
{
	LPBYTE buffer = NULL;
	DWORD needed = 0;
	DWORD returned = 0;
	DWORD err = 0;
	char tmp[1024];

	torture_comment(tctx, "Testing EnumPrinterDataEx(%s)", keyname);

	err = EnumPrinterDataEx(handle, keyname, NULL, 0, &needed, &returned);
	if (err == ERROR_MORE_DATA) {
		buffer = malloc(needed);
		torture_assert(tctx, buffer, "malloc failed");
		err = EnumPrinterDataEx(handle, keyname, buffer, needed, &needed, &returned);
	}
	if (err) {
		sprintf(tmp, "EnumPrinterDataEx(%s) failed on [%s] (buffer size = %d), error: %s\n",
			keyname, servername, needed, errstr(err));
		torture_fail(tctx, tmp);
	}

	if (tctx->print) {
		DWORD i;
		LPPRINTER_ENUM_VALUES v = (LPPRINTER_ENUM_VALUES)buffer;
		for (i=0; i < returned; i++) {
			print_printer_enum_values(&v[i]);
		}
	}

	if (returned_p) {
		*returned_p = returned;
	}

	if (buffer_p) {
		*buffer_p = buffer;
	} else {
		free(buffer);
	}

	return TRUE;
}


/****************************************************************************
****************************************************************************/

static BOOL test_OnePrinter(struct torture_context *tctx,
			    LPSTR printername,
			    LPSTR architecture,
			    LPPRINTER_DEFAULTS defaults)
{
	HANDLE handle;
	BOOL ret = TRUE;

	torture_comment(tctx, "Testing Printer %s", printername);

	ret &= test_OpenPrinter(tctx, printername, defaults, &handle);
	ret &= test_GetPrinter(tctx, printername, handle);
	ret &= test_GetPrinterDriver(tctx, printername, architecture, handle);
	ret &= test_EnumForms(tctx, printername, handle);
	ret &= test_EnumJobs(tctx, printername, handle);
	ret &= test_EnumPrinterKey(tctx, printername, handle, "");
	ret &= test_EnumPrinterKey(tctx, printername, handle, "PrinterDriverData");
	ret &= test_EnumPrinterDataEx(tctx, printername, "PrinterDriverData", handle, NULL, NULL);
	ret &= test_ClosePrinter(tctx, handle);

	return ret;
}

/****************************************************************************
****************************************************************************/

static BOOL test_EachPrinter(struct torture_context *tctx,
			     LPSTR servername,
			     LPSTR architecture,
			     LPPRINTER_DEFAULTS defaults)
{
	DWORD needed = 0;
	DWORD returned = 0;
	DWORD err = 0;
	char tmp[1024];
	DWORD i;
	DWORD flags = PRINTER_ENUM_NAME;
	PPRINTER_INFO_1 buffer = NULL;
	BOOL ret = TRUE;

	torture_comment(tctx, "Testing EnumPrinters level %d", 1);

	EnumPrinters(flags, servername, 1, NULL, 0, &needed, &returned);
	err = GetLastError();
	if (err == ERROR_INSUFFICIENT_BUFFER) {
		err = 0;
		buffer = (PPRINTER_INFO_1)malloc(needed);
		torture_assert(tctx, buffer, "malloc failed");
		if (!EnumPrinters(flags, servername, 1, (LPBYTE)buffer, needed, &needed, &returned)) {
			err = GetLastError();
		}
	}
	if (err) {
		sprintf(tmp, "EnumPrinters failed level %d on [%s] (buffer size = %d), error: %s\n",
			1, servername, needed, errstr(err));
		torture_fail(tctx, tmp);
	}

	for (i=0; i < returned; i++) {
		ret &= test_OnePrinter(tctx, buffer[i].pName, architecture, defaults);
	}

	free(buffer);

	return ret;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrintProcessorDirectory(struct torture_context *tctx,
					    LPSTR servername,
					    LPSTR architecture)
{
	DWORD levels[]  = { 1 };
	DWORD success[] = { 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing GetPrintProcessorDirectory level %d", levels[i]);

		GetPrintProcessorDirectory(servername, architecture, levels[i], NULL, 0, &needed);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!GetPrintProcessorDirectory(servername, architecture, levels[i], buffer, needed, &needed)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "GetPrintProcessorDirectory failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrinterDriverDirectory(struct torture_context *tctx,
					   LPSTR servername,
					   LPSTR architecture)
{
	DWORD levels[]  = { 1 };
	DWORD success[] = { 1 };
	DWORD i;
	LPBYTE buffer = NULL;

	for (i=0; i < ARRAY_SIZE(levels); i++) {

		DWORD needed = 0;
		DWORD err = 0;
		char tmp[1024];

		torture_comment(tctx, "Testing GetPrinterDriverDirectory level %d", levels[i]);

		GetPrinterDriverDirectory(servername, architecture, levels[i], NULL, 0, &needed);
		err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			err = 0;
			buffer = malloc(needed);
			torture_assert(tctx, buffer, "malloc failed");
			if (!GetPrinterDriverDirectory(servername, architecture, levels[i], buffer, needed, &needed)) {
				err = GetLastError();
			}
		}
		if (err) {
			sprintf(tmp, "GetPrinterDriverDirectory failed level %d on [%s] (buffer size = %d), error: %s\n",
				levels[i], servername, needed, errstr(err));
			if (success[i]) {
				torture_fail(tctx, tmp);
			} else {
				torture_warning(tctx, tmp);
			}
		}

		free(buffer);
		buffer = NULL;
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrinterData(struct torture_context *tctx,
				LPSTR servername,
				LPSTR valuename,
				HANDLE handle,
				DWORD *type_p,
				LPBYTE *buffer_p,
				DWORD *size_p)
{
	LPBYTE buffer = NULL;
	DWORD needed = 0;
	DWORD type;
	DWORD err = 0;
	char tmp[1024];

	torture_comment(tctx, "Testing GetPrinterData(%s)", valuename);

	err = GetPrinterData(handle, valuename, &type, NULL, 0, &needed);
	if (err == ERROR_MORE_DATA) {
		buffer = (LPBYTE)malloc(needed);
		torture_assert(tctx, buffer, "malloc failed");
		err = GetPrinterData(handle, valuename, &type, buffer, needed, &needed);
	}
	if (err) {
		sprintf(tmp, "GetPrinterData(%s) failed on [%s] (buffer size = %d), error: %s\n",
			valuename, servername, needed, errstr(err));
		torture_fail(tctx, tmp);
	}

	if (tctx->print) {
		print_printer_data("PrinterDriverData", valuename, needed, buffer, type);
	}

	if (type_p) {
		*type_p = type;
	}

	if (size_p) {
		*size_p = needed;
	}

	if (buffer_p) {
		*buffer_p = buffer;
	} else {
		free(buffer);
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_GetPrinterDataEx(struct torture_context *tctx,
				  LPSTR servername,
				  LPSTR keyname,
				  LPSTR valuename,
				  HANDLE handle,
				  DWORD *type_p,
				  LPBYTE *buffer_p,
				  DWORD *size_p)
{
	LPBYTE buffer = NULL;
	DWORD needed = 0;
	DWORD type;
	DWORD err = 0;
	char tmp[1024];

	torture_comment(tctx, "Testing GetPrinterDataEx(%s - %s)", keyname, valuename);

	err = GetPrinterDataEx(handle, keyname, valuename, &type, NULL, 0, &needed);
	if (err == ERROR_MORE_DATA) {
		buffer = (LPBYTE)malloc(needed);
		torture_assert(tctx, buffer, "malloc failed");
		err = GetPrinterDataEx(handle, keyname, valuename, &type, buffer, needed, &needed);
	}
	if (err) {
		sprintf(tmp, "GetPrinterDataEx(%s) failed on [%s] (buffer size = %d), error: %s\n",
			valuename, servername, needed, errstr(err));
		torture_fail(tctx, tmp);
	}

	if (tctx->print) {
		print_printer_data(keyname, valuename, needed, buffer, type);
	}

	if (type_p) {
		*type_p = type;
	}

	if (size_p) {
		*size_p = needed;
	}

	if (buffer_p) {
		*buffer_p = buffer;
	} else {
		free(buffer);
	}

	return TRUE;
}

/****************************************************************************
****************************************************************************/

static BOOL test_PrinterData(struct torture_context *tctx,
			     LPSTR servername,
			     HANDLE handle)
{
	BOOL ret = TRUE;
	DWORD i;
	DWORD type, type_ex;
	LPBYTE buffer, buffer_ex;
	DWORD size, size_ex;
	LPSTR valuenames[] = {
		SPLREG_DEFAULT_SPOOL_DIRECTORY,
		SPLREG_MAJOR_VERSION,
		SPLREG_MINOR_VERSION,
		SPLREG_DS_PRESENT,
		SPLREG_DNS_MACHINE_NAME,
		SPLREG_ARCHITECTURE,
		SPLREG_OS_VERSION
	};

	for (i=0; i < ARRAY_SIZE(valuenames); i++) {
		ret &= test_GetPrinterData(tctx, servername, valuenames[i], handle, &type, &buffer, &size);
		ret &= test_GetPrinterDataEx(tctx, servername, "random", valuenames[i], handle, &type_ex, &buffer_ex, &size_ex);
		torture_assert_int_equal(tctx, type, type_ex, "type mismatch");
		torture_assert_int_equal(tctx, size, size_ex, "size mismatch");
		torture_assert_mem_equal(tctx, buffer, buffer_ex, size, "buffer mismatch");
		free(buffer);
		free(buffer_ex);
	}

	return ret;
}

/****************************************************************************
****************************************************************************/

int main(int argc, char *argv[])
{
	BOOL ret = FALSE;
	LPSTR servername;
	LPSTR architecture = "Windows NT x86";
	HANDLE server_handle;
	PRINTER_DEFAULTS defaults_admin, defaults_use;
	struct torture_context *tctx;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <servername> [print]\n", argv[0]);
		exit(-1);
	}

	tctx = malloc(sizeof(struct torture_context));
	if (!tctx) {
		fprintf(stderr, "out of memory\n");
		exit(-1);
	}
	memset(tctx, '\0', sizeof(*tctx));

	servername = argv[1];

	if (argc >= 3) {
		if (strcmp(argv[2], "print") == 0) {
			tctx->print = TRUE;
		}
	}

	defaults_admin.pDatatype = NULL;
	defaults_admin.pDevMode = NULL;
	defaults_admin.DesiredAccess = PRINTER_ACCESS_ADMINISTER;

	defaults_use.pDatatype = NULL;
	defaults_use.pDevMode = NULL;
	defaults_use.DesiredAccess = PRINTER_ACCESS_USE;

	ret &= test_EnumPrinters(tctx, servername);
	ret &= test_EnumDrivers(tctx, servername, architecture);
	ret &= test_OpenPrinter(tctx, servername, NULL, &server_handle);
/*	ret &= test_EnumPrinterKey(tctx, servername, server_handle, ""); */
	ret &= test_PrinterData(tctx, servername, server_handle);
	ret &= test_EnumForms(tctx, servername, server_handle);
	ret &= test_ClosePrinter(tctx, server_handle);
	ret &= test_EnumPorts(tctx, servername);
	ret &= test_EnumMonitors(tctx, servername);
	ret &= test_EnumPrintProcessors(tctx, servername, architecture);
	ret &= test_EnumPrintProcessorDatatypes(tctx, servername);
	ret &= test_GetPrintProcessorDirectory(tctx, servername, architecture);
	ret &= test_GetPrinterDriverDirectory(tctx, servername, architecture);
	ret &= test_EachPrinter(tctx, servername, architecture, NULL);

	if (!ret) {
		if (tctx->last_reason) {
			fprintf(stderr, "failed: %s\n", tctx->last_reason);
		}
		free(tctx);
		return -1;
	}

	printf("%s run successfully\n", argv[0]);

	free(tctx);
	return 0;
}
