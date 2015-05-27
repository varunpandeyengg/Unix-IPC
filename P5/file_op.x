
struct file_result {
        string data<>;
};

program PRINTER {
	version PRINTER_V1 {
		file_result READ_FILE(string<>) = 1;
		file_result DELETE_FILE(string<>) = 2;
	} = 1;
} = 0x2fffffff;
