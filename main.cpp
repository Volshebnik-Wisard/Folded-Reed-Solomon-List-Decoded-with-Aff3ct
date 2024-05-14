#include <cstdlib>

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <aff3ct.hpp>
using namespace aff3ct;

struct params
{
	int   K = 233;     // number of information bits
	int   N = 255;     // codeword size
	int   m = 8;       // folding parameter
	int   t = (int)(N - K) / 2.;   // correction power
	int   fe = 100;     // number of frame errors
	int   seed = 0;     // PRNG seed for the AWGN channel
	float ebn0_min = 0.00f; // minimum SNR value
	float ebn0_max = 15.00f; // maximum SNR value
	float ebn0_step = 0.50f; // SNR step
	float R;                   // code rate (R=K/N)
	RS_polynomial_generator Pol_Gen{ (1 << (int)std::ceil(std::log2(N))) - 1, t };
};
void init_params(params& p);

struct modules
{
	std::unique_ptr<module::Source_random<>>                   source;
	std::unique_ptr<module::Encoder_RS<>>                     encoder;

	std::unique_ptr<tools::Interleaver_core_column_row<>> itl_col_row;
	
	std::unique_ptr<module::Interleaver<int>>                 itl_bit;
	std::unique_ptr<module::Interleaver<float>>               itl_llr;

	std::unique_ptr<tools::Constellation_QAM<>>              cstl_QAM;
	std::unique_ptr<module::Modem_generic<>>                modem_QAM;
	std::unique_ptr<module::Modem_BPSK<>>                  modem_BPSK;

	std::unique_ptr<module::Channel_AWGN_LLR<>>               channel;
	std::unique_ptr<module::Decoder_RS_std<>>             decoder_std;
	//std::unique_ptr<module::Decoder_RS_genius<>>         decoder_RS;
	//std::unique_ptr<module::Decoder_FRS<>>              decoder_FRS;

	std::unique_ptr<module::Monitor_BFER<>>                   monitor;

};
void init_modules(const params& p, modules& m);

struct buffers
{
	std::vector<int  > ref_bits;
	std::vector<int  > enc_bits;
	std::vector<int  > enc_bits_it;

	std::vector<float> symbols;
	std::vector<float> sigma;
	std::vector<float> noisy_symbols;

	std::vector<float> LLRs_it;
	std::vector<float> LLRs;
	std::vector<int  > dec_bits;
};
void init_buffers(const params& p, buffers& b);

struct utils
{
	std::unique_ptr<tools::Sigma<>>                 noise;     // a sigma noise type
	std::vector<std::unique_ptr<tools::Reporter>>   reporters; // list of reporters dispayed in the terminal
	std::unique_ptr<tools::Terminal_std>            terminal;  // manage the output text in the terminal

};
void init_utils(const modules& m, utils& u);



int main(int argc, char** argv)
{
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
		std::to_string(tools::version_minor()) + "." +
		std::to_string(tools::version_release());

	std::cout << "#----------------------------------------------------------" << std::endl;
	std::cout << "# This is a basic program using the AFF3CT library (" << v << ")" << std::endl;
	std::cout << "# Feel free to improve it as you want to fit your needs." << std::endl;
	std::cout << "#----------------------------------------------------------" << std::endl;
	std::cout << "#" << std::endl;

	params p;  init_params(p); // create and initialize the parameters defined by the user
	modules m; init_modules(p, m); // create and initialize the modules
	buffers b; init_buffers(p, b); // create and initialize the buffers required by the modules
	utils u; init_utils(m, u); // create and initialize the utils


	// display the legend in the terminal
	u.terminal->legend();

	// loop over the various SNRs
	for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step)
	{
		// compute the current sigma for the channel noise
		const auto esn0 = tools::ebn0_to_esn0(ebn0, p.R);


		std::fill(b.sigma.begin(), b.sigma.end(), tools::esn0_to_sigma(esn0));
		u.noise->set_values(b.sigma[0], ebn0, esn0);
		// display the performance (BER and FER) in real time (in a separate thread)
		u.terminal->start_temp_report();
		// run the simulation chain
		while (!m.monitor->fe_limit_achieved() && !u.terminal->is_interrupt())
		{
			m.source->generate(b.ref_bits);
			m.encoder->encode(b.ref_bits, b.enc_bits);
			//b.enc_bits_it = foldingint(b.enc_bits, p.m);
			m.itl_bit->interleave(b.enc_bits, b.enc_bits_it);
			//m.modem_BPSK->modulate(b.enc_bits_it, b.symbols);
			m.modem_QAM->modulate(b.enc_bits_it, b.symbols);
			m.channel->add_noise(b.sigma, b.symbols, b.noisy_symbols);
			m.modem_QAM->demodulate(b.sigma, b.noisy_symbols, b.LLRs_it);
			//m.modem_BPSK->demodulate(b.sigma, b.noisy_symbols, b.LLRs_it);
			m.itl_llr->deinterleave(b.LLRs_it, b.LLRs);
			//ListDecoder::decode(b.LLRs, b.dec_bits,p.N,p.K, 1 << (int)std::ceil(std::log2(p.N)) - 1,p.Bin_Field);
			m.decoder_std->decode_siho(b.LLRs, b.dec_bits);
			//m.decoder_RS->decode_siho(b.LLRs, b.dec_bits);
			m.monitor->check_errors(b.dec_bits, b.ref_bits);
		}

		// display the performance (BER and FER) in the terminal
		u.terminal->final_report();

		// reset the monitor for the next SNR
		m.monitor->reset();
		u.terminal->reset();

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (u.terminal->is_over()) break;
	}

	//std::vector<std::vector<int>> bit1 = folding(b.enc_bits, p.m);
	//std::vector<int> bit2 = unfolding(bit1, p.m);

	std::cout << "# End of the simulation" << std::endl;

	return 0;
}

void init_params(params& p)
{
	p.R = (float)p.K / (float)p.N;
	std::cout << "# * Simulation parameters: " << std::endl;
	std::cout << "#    ** Correction power = " << p.t << std::endl;
	std::cout << "#    ** Noise seed       = " << p.seed << std::endl;
	std::cout << "#    ** Frame errors     = " << p.fe << std::endl;
	std::cout << "#    ** Noise seed       = " << p.seed << std::endl;
	std::cout << "#    ** Info. bits   (K) = " << p.K << std::endl;
	std::cout << "#    ** Frame size   (N) = " << p.N << std::endl;
	std::cout << "#    ** Folding par. (m) = " << p.m << std::endl;
	std::cout << "#    ** Code rate    (R) = " << p.R << std::endl;
	std::cout << "#    ** SNR min     (dB) = " << p.ebn0_min << std::endl;
	std::cout << "#    ** SNR max     (dB) = " << p.ebn0_max << std::endl;
	std::cout << "#    ** SNR step    (dB) = " << p.ebn0_step << std::endl;
	std::cout << "#" << std::endl;
}

void init_modules(const params& p, modules& m)
{

	m.source = std::unique_ptr <module::Source_random        <>>(new module::Source_random    <>(p.K * 8));
	m.encoder = std::unique_ptr<module::Encoder_RS           <>>(new module::Encoder_RS       <>(p.K, p.N, p.Pol_Gen));

	m.itl_col_row = std::unique_ptr<tools::Interleaver_core_column_row<>>(new tools::Interleaver_core_column_row<>(p.N * 8, p.m, "TOP_LEFT"));
	m.itl_bit = std::unique_ptr<module::Interleaver       <int>>(new module::Interleaver   <int>(*m.itl_col_row));
	m.itl_llr = std::unique_ptr<module::Interleaver     <float>>(new module::Interleaver <float>(*m.itl_col_row));

	m.cstl_QAM = std::unique_ptr<tools::Constellation_QAM    <>>(new tools::Constellation_QAM <>(p.m));
	m.modem_BPSK = std::unique_ptr<module::Modem_BPSK        <>>(new module::Modem_BPSK       <>(p.N * 8));
	m.modem_QAM = std::unique_ptr <module::Modem_generic     <>>(new module::Modem_generic    <>(p.N * 8, *m.cstl_QAM));

	m.channel = std::unique_ptr<module::Channel_AWGN_LLR     <>>(new module::Channel_AWGN_LLR <>(p.N * 8 / p.m * 2));
	m.decoder_std = std::unique_ptr<module::Decoder_RS_std   <>>(new module::Decoder_RS_std   <>(p.K, p.N, p.Pol_Gen));
	//m.decoder_RS = std::unique_ptr<module::Decoder_RS_genius <>>(new module::Decoder_RS_genius<>(p.K, p.N, p.Pol_Gen, *m.encoder));

	m.monitor = std::unique_ptr<module::Monitor_BFER      <>>(new module::Monitor_BFER     <>(p.K * 8, p.fe));
	m.channel->set_seed(p.seed);
};

void init_buffers(const params& p, buffers& b)
{
	b.ref_bits = std::vector       <int>(p.K * 8);
	b.enc_bits = std::vector       <int>(p.N * 8);
	b.enc_bits_it = std::vector    <int>(p.N * 8);

	b.symbols = std::vector      <float>(p.N * 8 / p.m * 2);
	b.sigma = std::vector        <float>(1);
	b.noisy_symbols = std::vector<float>(p.N * 8 / p.m * 2);

	b.LLRs_it = std::vector      <float>(p.N * 8);
	b.LLRs = std::vector         <float>(p.N * 8);
	b.dec_bits = std::vector       <int>(p.K * 8);
}

void init_utils(const modules& m, utils& u)
{

	// create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	//u.RS_Polynomial_generator = std::unique_ptr<tools::RS_polynomial_generator>(new tools::RS_polynomial_generator(p.N_pol, p.t));
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*m.monitor)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*m.monitor)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal_std>(new tools::Terminal_std(u.reporters));

}


