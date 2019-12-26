#include "helpers.hpp"
#include "declarations.hpp"

bool ssh_send(string,string,string);
void write_for_urgent_transmission(string,string,string);

using namespace std;
/*

void modem_cleanse( string imei ) {
	map<string,string>::iterator it_modem_daemon = MODEM_DAEMON.find(imei);
	if(it_modem_daemon != MODEM_DAEMON.end()) 
		MODEM_DAEMON.erase( it_modem_daemon );
}


vector<string> get_modems_jobs(string folder_name) {
	return helpers::split( helpers::terminal_stdout((string)("ls -1 " + folder_name)), '\n', true );
}

map<string, string> read_request_file( string full_filename, string modem_imei) {
	string func_name = "read_request_file";
	printf("%s=> EXECUTING JOB FOR FILE: %s\n", func_name.c_str(), full_filename.c_str());
	ifstream read_job(full_filename.c_str());
	if(!read_job) {
		cerr << func_name << "=> error reading job: " << full_filename << endl;
		return (map<string,string>){};
	}

	string tmp_buffer, number, message = "";
	short int line_counter = 0;

	//Why is this doing this here??????
	//I see... some messages have \n characters in them... this part is to make sure they maintain that integrity
	while(getline(read_job, tmp_buffer)) {
		if(line_counter == 0) number = tmp_buffer;
		else if(line_counter > 0) {
			if( line_counter == 1) message = tmp_buffer;
			else message += "\n" + tmp_buffer;
		}
		++line_counter;
	}
	read_job.close();

	map<string,string> message_tuple = {{"message", message}, {"number", number}};
	if( message.empty() or number.empty()) return map<string,string>{};
	
	return message_tuple;
}



bool mmcli_send( string message, string number, string modem_index ) {
	string func_name = "mmcli_send";
	string sms_command = "./modem_information_extraction.sh sms send \"" + helpers::remove_carriage( message ) + "\" " + number + " " + modem_index;
	string terminal_stdout = helpers::terminal_stdout(sms_command);
	cout << func_name << "=> sending sms message...\n" << func_name << "=> \t\tStatus " << terminal_stdout << endl << endl;
	if(terminal_stdout.find("success") == string::npos and terminal_stdout.find("Success") == string::npos) {
		if(terminal_stdout.find("timed out") != string::npos) {
			fprintf(stderr, "%s=> Modem needs to sleep... going down for %d seconds\n", func_name.c_str(), GL_MMCLI_MODEM_SLEEP_TIME);
			std::this_thread::sleep_for(std::chrono::seconds(GL_MMCLI_MODEM_SLEEP_TIME));
			return true;
		}
		cerr << func_name << "=> MMCLI send failed" << endl;
		return false;
	}

	cout << func_name << "=> MMCLI send success" << endl;
	return true;
}

void update_modem_success_count( string modem_imei ) {
	//TODO: increment success count for this modem
	if( GL_SUCCESS_MODEM_LIST.find( modem_imei ) == GL_SUCCESS_MODEM_LIST.end() ) {
		GL_SUCCESS_MODEM_LIST.insert( make_pair( modem_imei, 0 ) );
	}

	GL_SUCCESS_MODEM_LIST[modem_imei] += 1;
}

bool ssh_send( string message, string number, string modem_ip ) {
	//TODO: Figure out how to make SSH tell if SMS has gone out or failed
	string func_name = "ssh_send";
	string sms_command = "ssh root@" + modem_ip + " -T -o \"ConnectTimeout=20\" \"sendsms '" + number + "' \\\"" + helpers::remove_carriage( message ) + "\\\"\"";
	string terminal_stdout = helpers::terminal_stdout(sms_command);
	cout << func_name << "=> sending sms message...\n" << func_name << "=> \t\tStatus " << terminal_stdout << endl << endl;

	if(terminal_stdout.find("failed") != string::npos ) {
		cerr << func_name << "=> ssh failed to send message! Doing emergency re-routing..." << endl;
		write_for_urgent_transmission( modem_ip, message, number );
	}

	return true; //FIXME: This is propaganda
}

void write_for_urgent_transmission( string modem_imei, string message, string number ) {
	//XXX: which modem has been the most successful
	string func_name = "write_for_urgent_transmission";
	if( !GL_SUCCESS_MODEM_LIST.empty() ) {
		string most_successful_modem;
		auto it_GL_SUCCESS_MODEM_LIST = GL_SUCCESS_MODEM_LIST.begin();
		int most_successful_modem_count = it_GL_SUCCESS_MODEM_LIST->second;
		string isp = MODEM_DAEMON[modem_imei];
		++it_GL_SUCCESS_MODEM_LIST;

		//FIXME: Something's wrong with this iterator
		for( auto it_GL_SUCCESS_MODEM_LIST : GL_SUCCESS_MODEM_LIST ) {
			if( it_GL_SUCCESS_MODEM_LIST.first != modem_imei and it_GL_SUCCESS_MODEM_LIST.second >= most_successful_modem_count and helpers::to_upper(MODEM_DAEMON[it_GL_SUCCESS_MODEM_LIST.first]).find( helpers::to_upper(isp) ) != string::npos ) {
				most_successful_modem_count = it_GL_SUCCESS_MODEM_LIST.second;
				most_successful_modem = it_GL_SUCCESS_MODEM_LIST.first;
			}
		}
		printf("%s=> Most successful modem | %s | count | %d\n", func_name.c_str(), most_successful_modem.c_str(), most_successful_modem_count);
		if( most_successful_modem.empty() ) {
			//FIXME: Should check for another modem rather than send things back to the request file
			fprintf( stderr, "%s=> No modem found for emergency transmission... writing back to request file\n", func_name.c_str() );
			helpers::write_to_request_file( message, number );
			return;
		}

		string modem_index;
		bool ssh_modem = is_ssh_modem( most_successful_modem );
		if( !ssh_modem ) {
			for(auto modem_details : MODEM_POOL) {
				if( modem_details.second[0] == most_successful_modem ) {
					modem_index = modem_details.first;
					break;
				}
			}
		}
		else {
			modem_index = most_successful_modem;
		}

		//FIXME: This solution is not checking for SSH modems
		if( modem_index.empty()) {
			//FIXME: Should check for another modem rather than send things back to the request file
			helpers::write_to_request_file( message, number );
		}
		else {
			helpers::write_modem_job_file( modem_index, message, number );
		}
	}
	else {
		helpers::write_to_request_file( message, number );
	}
}


void write_modem_details( string modem_imei, string isp ) {
	string func_name = "write_modem_details";
	printf("%s=> Writing modem details to modem [%s] file...\n", func_name.c_str(), modem_imei.c_str());
	ofstream write_modem_file( SYS_FOLDER_MODEMS + "/" + modem_imei + "/.details.dek", ios::trunc);
	write_modem_file << isp << endl;
	write_modem_file.close();
}

void modem_listener(string func_name, string modem_imei, string modem_index, string ISP, bool watch_dog = true, string type = "MMCLI") {
	//XXX: Just 1 instance should be running for every modem_imei
	printf("%s=> Started instance of modem=> \n+imei[%s] +index[%s] +isp[%s] +type[%s]\n\n", func_name.c_str(), modem_imei.c_str(), modem_index.c_str(), ISP.c_str(), type.c_str());

	//XXX: Log details of this instance
	write_modem_details( modem_imei, ISP );

	MODEM_DAEMON.insert(make_pair(modem_imei, ISP));
	while(GL_MODEM_LISTENER_STATE) {
		if(watch_dog) {
			if(!helpers::modem_is_available( modem_imei ) ) {
				printf("%s=> Killed instance of modem because disconncted\n\t+imei[%s] +index[%s] +isp[%s] +type[%s]\n", func_name.c_str(), modem_imei.c_str(), modem_index.c_str(), ISP.c_str(), type.c_str());
				modem_cleanse( modem_imei );
				break;
			}
		}
		//read the modems folder for changes
		vector<string> jobs = get_modems_jobs((string)(SYS_FOLDER_MODEMS + "/" + modem_imei));
		printf("%s=> [%lu] found jobs...\n", func_name.c_str(), jobs.size());
		for(auto filename : jobs) {
			string full_filename = SYS_FOLDER_MODEMS + "/" + modem_imei + "/" + filename;
			map<string,string> request_file_content = read_request_file( full_filename, modem_imei );

			if( request_file_content.empty() ) {
				fprintf(stderr, "%s=> Nothing returned from file....", func_name.c_str() );
				continue;
			}
			if( request_file_content["message"].empty() ) {
				fprintf( stderr, "%s=> Found bad file --- no message--- deleting....", func_name.c_str());
				if( remove(full_filename.c_str()) != 0 ) {
					cerr << func_name << "=> failed to delete job!!!!!" << endl;
					char str_error[256];
					cerr << func_name << "=> errno message: " << strerror_r(errno, str_error, 256) << endl;
				}
				continue;
			}
			if( request_file_content["number"].empty() ) {
				fprintf( stderr, "%s=> Found bad file --- no number--- deleting....", func_name.c_str());
				if( remove(full_filename.c_str()) != 0 ) {
					cerr << func_name << "=> failed to delete job!!!!!" << endl;
					char str_error[256];
					cerr << func_name << "=> errno message: " << strerror_r(errno, str_error, 256) << endl;
				}
				continue;
			}

			string message = request_file_content["message"];
			string number = request_file_content["number"];

			//printf("%s=> processing job: number[%s], message[%s]\n", func_name.c_str(), number.c_str(), message.c_str());
			if(type == "MMCLI") {
				mmcli_send( message, number, modem_index ) ? update_modem_success_count( modem_imei ) : write_for_urgent_transmission( modem_imei, message, number );
			}

			else if(type == "SSH") {
				ssh_send( message, number, modem_index ) ? update_modem_success_count( modem_imei ) : write_for_urgent_transmission( modem_imei, message, number );
			}

			//XXX: Test if it fails to delete this file
			if( remove(full_filename.c_str()) != 0 ) {
				cerr << func_name << "=> failed to delete job!!!!!" << endl;
				char str_error[256];
				cerr << func_name << "=> errno message: " << strerror_r(errno, str_error, 256) << endl;
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(GL_TR_SLEEP_TIME));
	}

	return;
}


void ssh_extractor( string ip_gateway ) {
	string func_name = "configure_ssh_modems";

	//verify ssh is actually a modem
	string ssh_stdout = helpers::terminal_stdout((string)("ssh root@" + ip_gateway + " -T -o \"ConnectTimeout=10\" deku"));

	//cout << func_name << "=> ssh output: " << ssh_stdout << endl;
	vector<string> ssh_stdout_lines = helpers::split(ssh_stdout, '\n', true);
	if(ssh_stdout_lines[0] == "deku:verified:") {
		cout << func_name << "=> SSH server ready for SMS!" << endl;
		if(mkdir((char*)(SYS_FOLDER_MODEMS + "/" + ip_gateway).c_str(), STD_DIR_MODE) != 0 && errno != EEXIST) {
			char str_error[256];
			cerr << func_name << ".error=> " << strerror_r(errno, str_error, 256) << endl;
		}
		else {
			if(MODEM_DAEMON.find(ip_gateway) != MODEM_DAEMON.end()) {
				cout << func_name << "=> instance of SSH already running... watch dog reset!" << endl;
				std::this_thread::sleep_for(std::chrono::seconds(GL_TR_SLEEP_TIME));
				return;
			}

			std::thread tr_ssh_listener(modem_listener, "\tSSH Listener", ip_gateway, ip_gateway, ssh_stdout_lines[1], true, "SSH");
			tr_ssh_listener.detach();
		}
	}
	else {
		cout << func_name << "=> Could not verify SSH server!" << endl;
	}

	return;
}

vector<string> extract_modem_details ( string modem_imei ) {
	string func_name = "extracat_modem_details";

	ifstream read_modem_file ( SYS_FOLDER_MODEMS + "/" + modem_imei + "/.details.dek");
	vector<string> file_details;
	
	if( !read_modem_file.good()) {
		cerr << func_name << "=> No detail file for modem " << modem_imei << endl;
	}

	else {
		string tmp_string;
		while( getline( read_modem_file, tmp_string ) ) file_details.push_back( tmp_string );
	}
	
	read_modem_file.close();
	
	return file_details;
}
*/

string read_modem_details( string modem_imei ) {
	vector<string> info = helpers::read_file( SYS_FOLDER_MODEMS + "/" + modem_imei + "/.details.dek" );
	return info.empty() ? "" : info[0];
}



map<string,string> modem_extractor( map<string,string> modem_meta_info ) {
	string func_name = "modem_extractor";
	map<string,string> modem_info;

	if( modem_meta_info["type"] == "mmcli" ) {
		string str_stdout = helpers::terminal_stdout( GET_MODEM_INFO() + " extract " + modem_meta_info["index"] );
		logger::logger( func_name, "\n" + str_stdout + "\n" );
		string modem_service_provider = "";

		vector<string> modem_information = helpers::split(str_stdout, '\n', true);
		string modem_imei = helpers::split(modem_information[0], ':', true)[1];
		string modem_sig_quality = helpers::split(modem_information[1], ':', true)[1];
		logger::logger( func_name, "\nimei: " + modem_imei + "\nmodem_sig_qual: " + modem_sig_quality + "\n");

		modem_meta_info.insert(make_pair ( "imei", modem_imei ));
		modem_meta_info.insert(make_pair ( "signal_quality", modem_sig_quality ));

		if(modem_information.size() != 3 or helpers::split(modem_information[2], ':', true).size() < 2) {
			//std::this_thread::sleep_for(std::chrono::seconds(GL_TR_SLEEP_TIME));
			//printf("%s=> modem information extracted - incomplete [%lu]\n", func_name.c_str(), modem_information.size());
			logger::logger(func_name, "modem information not available for extraction\n", "stderr", true);
			if( string modem_isp = read_modem_details( modem_imei ); !modem_isp.empty() ) {
				logger::logger( func_name, "extracting from details file..." );
				modem_meta_info.insert( make_pair ( "isp", modem_isp ));
			}
			else {
				logger::logger( func_name, "No detail file, manually create if needed", "stdout", true);
				return modem_info;
			}
		}
		else {
			modem_service_provider = helpers::split(modem_information[2], ':', true)[1];
			modem_meta_info.insert( make_pair( "isp", modem_service_provider ));
		}
	}
	else if( modem_meta_info["type"] == "ssh" ) {
		string str_stdout = helpers::terminal_stdout( "ssh -T root@" + modem_meta_info["index"] + " deku");
		logger::logger( func_name, "\n" + str_stdout + "\n" );

		vector<string> modem_information = helpers::split(str_stdout, '\n', true);
		if( modem_information.size() > 1 and modem_information[0] == "deku:verified:" ) {
			modem_meta_info.insert( make_pair( "imei", modem_meta_info["index"] ));
			modem_meta_info.insert( make_pair( "isp", modem_information[1] ));
		}
		else {
			logger::logger( func_name, "could not verify SSH modem\n", "stderr", true);
			return modem_info;
		}
	}
	else {
		logger::logger( func_name, "\nnot a valid type of modem\n", "stderr", true);
		return modem_info;
	}

	return modem_meta_info;
}

/*
void daemon_start_modem_listener() {
	vector<map<string,string>> list_of_modems = gl_modem_listener();
	
}
*/