#include "wstp.h"
#include <iostream>

// https://reference.wolfram.com/legacy/language/v10.2/ref/c/WSLogStreamToFile.html
// http://edenwaith.com/development/tutorials/mathlink/ML_Tut.pdf
//
// Run as
//
//    core/test_wstp -linkname 'math -mathlink' -linkmode launch


int main(int argc, char **argv)
	{
	int errno;
	WSEnvironment stdenv;
	stdenv = WSInitialize((WSEnvironmentParameter)0);
	if(stdenv==0) {
		std::cerr << "Failed to initialise WSTP" << std::endl;
		return -1;
		}
	//	auto lp = WSLoopbackOpen(stdenv, &errno);
	auto lp = WSOpenArgcArgv(stdenv, argc, argv, &errno);
	if(lp==0 || errno!=WSEOK) {
		std::cerr << "Failed to open loopback link " << WSErrorMessage(lp) << std::endl;
		return -1;
		}

	WSActivate(lp);

	const char *fileName;
	auto res = WSLogFileNameForLink(lp, &fileName);
	if(! res) {
		std::cerr << "Failed to generate log file name" << std::endl;
		return -1;
		}
	res = WSLogStreamToFile(lp, fileName);
	std::cerr << "Logging to " << fileName << std::endl;

	WSPutFunction(lp, "EvaluatePacket", 1L);
	WSPutFunction(lp, "ToString", 1L);
	WSPutFunction(lp, "FullSimplify", 1L);
	WSPutFunction(lp, "ToExpression", 1L);
	WSPutString(lp, "Sin[x]^2 + Cos[x]^2 + Cos[x] Sin[x]");
	//	WSPutFunction(lp, "Plus", 2);
	//	WSPutInteger(lp, 3);
	//	WSPutInteger(lp, 8);
	WSEndPacket(lp);
	WSFlush(lp);

	int pkt=0;
	while( (pkt = WSNextPacket(lp), pkt) && pkt != RETURNPKT ) {
		std::cerr << "received packet " << pkt << std::endl;
		WSNewPacket(lp);
		if (WSError(lp)) {
			std::cerr << "error" << std::endl;
			}
		}
	std::cerr << "packet now " << pkt << std::endl;

	const char *out;
	if(! WSGetString(lp, &out)) {
		printf("Unable to read from link\n");
		return -1;
		} else {
		std::cerr << out << std::endl;
		WSReleaseString(lp, out);
		}

	WSReleaseLogFileNameForLink(lp, fileName);

	WSClose(lp);
	WSDeinitialize(stdenv);
	}
