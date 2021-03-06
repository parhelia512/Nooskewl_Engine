#include "Nooskewl_Engine/engine.h"
#include "Nooskewl_Engine/internal.h"
#include "Nooskewl_Engine/mml.h"
#include "Nooskewl_Engine/sample.h"

using namespace Nooskewl_Engine;

Sample *MML::bass_drum;
Sample *MML::hihat;

std::vector<MML::Internal *> MML::loaded_mml;

void MML::start()
{
	bass_drum = new Sample("bass_drum.wav");
	hihat = new Sample("hihat.wav");
}

void MML::end()
{
	delete bass_drum;
	delete hihat;
}

void MML::pause_all()
{
	for (size_t i = 0; i < loaded_mml.size(); i++) {
		for (size_t j = 0; j < loaded_mml[i]->tracks.size(); j++) {
			loaded_mml[i]->tracks[j]->pause();
		}
	}
}

#define TWO_PI (2.0f * (float)M_PI)
#define STREAM_FREQUENCY 44100
#define TO_INT16(f) ((short)(f*32767))

const float dt = 1.0f / STREAM_FREQUENCY;

const float note_pitches[12][11] = {
	{ 16.352f, 32.703f, 65.406f, 130.81f, 261.63f, 523.25f, 1046.5f, 2093.0f, 4186.0f, 8372.0f, 16744.0f },
	{ 17.324f, 34.648f, 69.296f, 138.59f, 277.18f, 554.37f, 1108.7f, 2217.5f, 4434.9f, 8869.8f, 17739.7f },
	{ 18.354f, 36.708f, 73.416f, 146.83f, 293.66f, 587.33f, 1174.7f, 2349.3f, 4698.6f, 9397.3f, 18794.5f },
	{ 19.445f, 38.891f, 77.782f, 155.56f, 311.13f, 622.25f, 1244.5f, 2489.0f, 4978.0f, 9956.1f, 19912.1f },
	{ 20.602f, 41.203f, 82.407f, 164.81f, 329.63f, 659.26f, 1318.5f, 2637.0f, 5274.0f, 10548.1f, 21096.2f },
	{ 21.827f, 43.654f, 87.307f, 174.61f, 349.23f, 698.46f, 1396.9f, 2793.8f, 5587.7f, 11175.3f, 22350.6f },
	{ 23.125f, 46.249f, 92.499f, 185.00f, 369.99f, 739.99f, 1480.0f, 2960.0f, 5919.9f, 11839.8f, 23679.6f },
	{ 24.500f, 48.999f, 97.999f, 196.00f, 392.00f, 783.99f, 1568.0f, 3136.0f, 6271.9f, 12543.9f, 25087.7f },
	{ 25.957f, 51.913f, 103.83f, 207.65f, 415.30f, 830.61f, 1661.2f, 3322.4f, 6644.9f, 13289.8f, 26579.5f },
	{ 27.500f, 55.000f, 110.00f, 220.00f, 440.00f, 880.00f, 1760.0f, 3520.0f, 7040.0f, 14080.0f, 28160.0f },
	{ 29.135f, 58.270f, 116.54f, 233.08f, 466.16f, 932.33f, 1864.7f, 3729.3f, 7458.6f, 14917.2f, 29834.5f },
	{ 30.868f, 61.735f, 123.47f, 246.94f, 493.88f, 987.77f, 1975.5f, 3951.1f, 7902.1f, 15804.3f, 31608.5f }
};

const char indexes[7] = {
	9, 11, 0, 2, 4, 5, 7 // a, b, c, d, e, f, g
};

static std::string token(const char *text, int *pos)
{
	char tok[100];
	int i = 0;
	
	while (*(text+*pos) != 0 && isspace(*(text+*pos))) (*pos)++; // skip whitespace

	// Read the token
	if (*(text+*pos) != 0) {
		tok[i++] = *(text+*pos);
		(*pos)++;
	}

	while (*(text+*pos) != 0 && (isdigit(*(text+*pos)) || *(text+*pos) == '+' || *(text+*pos) == '-' || *(text+*pos) == '.' || (tok[0] == '@' && *(text+*pos) >= 'A' && *(text+*pos) <= 'Z'))) {
		tok[i++] = *(text+*pos);
		(*pos)++;
	}

	while (*(text+*pos) != 0 && isspace(*(text+*pos))) (*pos)++; // skip more whitespace

	tok[i] = 0;
	return tok;
}

// In samples
static int onenotelength(const char *tok, int note_length, int tempo, int octave, int note, char prev_ch, std::vector<int> &pitches, std::vector< std::vector<float> > &pitch_envelopes)
{
	char ch;
	if (*tok == 'w') {
		ch = prev_ch;
	}
	else {
		ch = *tok;
	}
	int index = -1;
	if (ch >= 'a' && ch <= 'g') {
		index = indexes[ch - 'a'];
		tok++;
		if (*tok == '+') {
			index++;
			tok++;
		}
		else if (*tok == '-') {
			index--;
			tok++;
		}
	}
	else {
		tok++;
	}
	int length = atoi(tok);
	float total = (STREAM_FREQUENCY / (tempo / 4.0f / 60.0f)) / (length == 0 ? note_length : length);
	float dotlength = total / 2.0f;
	while (*tok != 0) {
		if (*tok == '.') {
			total += dotlength;
			dotlength /= 2.0f;
		}
		tok++;
	}
	return (int)total;
}

MML::Internal::Track::Track(Type type, std::string text, std::vector< std::pair<int, float> > &volumes, std::vector<int> &pitches, std::vector< std::vector<float> > &pitch_envelopes, std::vector< std::pair<int, float> > &dutycycles, int pad) :
	type(type),
	text(text),
	volumes(volumes),
	pitches(pitches),
	pitch_envelopes(pitch_envelopes),
	dutycycles(dutycycles),
	pad(pad),
	playing(false),
	paused(false)
{
	reset();
}

MML::Internal::Track::~Track()
{
}

void MML::Internal::Track::play(bool loop)
{
	done = false;

	paused = false;

	playing = true;

	this->loop = loop;
}

void MML::Internal::Track::stop()
{
	paused = false;
	playing = false;
}

void MML::Internal::Track::pause()
{
	paused = true;
	playing = false;
}

bool MML::Internal::Track::update(short *buf, int length)
{
	if (done) {
		return false;
	}

	while (buffer_fulfilled < length) {
		int to_generate = length_in_samples - note_fulfilled;
		int left_in_buffer = length - buffer_fulfilled;
		int fulfilled = buffer_fulfilled;
		if (left_in_buffer <= to_generate) {
			to_generate = left_in_buffer;
			fulfilled = length;
		}
		else {
			fulfilled += to_generate;
		}
		generate(buf + buffer_fulfilled, length_in_samples, to_generate, t, tok.c_str(), octave);
		buffer_fulfilled = fulfilled;
		bool get_next_note = false;
		if (note_fulfilled >= length_in_samples) {
			get_next_note = true;
		}
		if (get_next_note) {
			const char *text_cstr = text.c_str();

			std::string last_tok = tok;

			tok = next_note(text_cstr, &pos);

			if (tok.c_str()[0] != '0' && tok != last_tok) {
				t = 0;
			}

			note++;
			if (tok[0] == 0) {
				note--;
				if (padded) {
					if (loop) {
						reset();
					}
				}
				else {
					padded = true;
					if (loop) {
						length_in_samples = pad;
						tok = "r";
					}
					else {
						memset(buf, m.device_spec.silence, sizeof(short) * length);
						if (loop == false) {
							stop();
							reset();
						}
						// reset above sets done to false
						done = true;
						return true;
					}
				}
			}
			else {
				length_in_samples = notelength(tok.c_str(), text_cstr, &pos);
			}
			note_fulfilled = 0;
		}
		else {
			t += dt * to_generate;
		}
	}
	buffer_fulfilled = 0;

	return true;
}

bool MML::Internal::Track::is_playing()
{
	return playing;
}

bool MML::Internal::Track::is_done()
{
	return done;
}

void MML::Internal::Track::reset()
{
	dutycycle = 0.5f;
	octave = 4;
	note_length = 4;
	volume = 1.0f;
	tempo = 120;
	sample = 0;
	note = 0;
	volume_section = 0;
	dutycycle_section = 0;
	t = 0.0f;
	pos = 0;
	tok = next_note(text.c_str(), &pos);
	length_in_samples = notelength(tok.c_str(), text.c_str(), &pos);
	buffer_fulfilled = 0;
	note_fulfilled = 0;
	padded = false;
	done = false;
}

void MML::Internal::Track::pulse(short *buf, size_t samples, float t, float frequency, float phase)
{
	unsigned i;

	for (i = 0; i < samples; i++) {
		float w = TWO_PI * get_frequency(frequency);
		float ti = t + i * dt;
		float a = fmod(w * ti + phase, TWO_PI);
		float x;

		if (a < TWO_PI*get_dutycycle()) {
			x = 1;
		}
		else {
			x = -1;
		}

		buf[i] = TO_INT16(x * get_volume());

		sample++;
		note_fulfilled++;
	}
}

void MML::Internal::Track::noise(short *buf, size_t samples, float t, float frequency, float phase)
{
	unsigned i;

	for (i = 0; i < samples; i++) {
		float w = TWO_PI * get_frequency(frequency);
		float ti = t + i * dt;
		float a = fmod(w * ti + phase, TWO_PI);

		float r = (rand() % 255 / 255.0f); // FIXME: use different rand

		float v = fmodf(a, TWO_PI) / TWO_PI * get_volume();

		buf[i] = TO_INT16(r * v);

		sample++;
		note_fulfilled++;
	}
}

void MML::Internal::Track::sawtooth(short *buf, size_t samples, float t, float frequency, float phase)
{
	unsigned i;

	for (i = 0; i < samples; i++) {
		float w = TWO_PI * get_frequency(frequency);
		float tx = w * (t + i * dt) + (float)M_PI + phase;
		float tu = fmod(tx/(float)M_PI, 2.0f);

		buf[i] = TO_INT16((-1.0f + tu) * get_volume());

		sample++;
		note_fulfilled++;
	}
}

void MML::Internal::Track::sine(short *buf, size_t samples, float t, float frequency, float phase)
{
	unsigned i;

	for (i = 0; i < samples; i++) {
		float w = TWO_PI * get_frequency(frequency);
		float ti = t + i * dt;
		buf[i] = TO_INT16(sin(w * ti + phase) * get_volume());

		sample++;
		note_fulfilled++;
	}
}

void MML::Internal::Track::triangle(short *buf, size_t samples, float t, float frequency, float phase)
{
	unsigned i;

	for (i = 0; i < samples; i++) {
		float w = TWO_PI * get_frequency(frequency);
		float tx = w * (t + i * dt) + (float)M_PI/2.0f + phase;
		float tu = fmod(tx/(float)M_PI, 2.0f);

		if (tu <= 1.0f)
			buf[i] = TO_INT16((1.0f - 2.0f * tu) * get_volume());
		else
			buf[i] = TO_INT16((-1.0f + 2.0f * (tu - 1.0f)) * get_volume());

		sample++;
		note_fulfilled++;
	}
}

void MML::Internal::Track::generate(short *buf, int length_in_samples, int samples, float t, const char *tok, int octave)
{
	char c = tok[0];
	int index = 0;
	if (c == 'r') {
		memset(buf, m.device_spec.silence, sizeof(short)*samples);
		sample += samples;
		note_fulfilled += samples;
		return;
	}
	else if (c >= 'a' && c <= 'g') {
		index = indexes[c-'a'];
		if (tok[1] == '+') index++;
		else if (tok[1] == '-') index--;
	}
	float pitch = note_pitches[index][octave];

	switch (type) {
		case PULSE:
			pulse(buf, samples, t, pitch, 0.0f);
			break;
		case NOISE:
			noise(buf, samples, t, pitch, 0.0f);
			break;
		case SAWTOOTH:
			sawtooth(buf, samples, t, pitch, 0.0f);
			break;
		case SINE:
			sine(buf, samples, t, pitch, 0.0f);
			break;
		case TRIANGLE:
			triangle(buf, samples, t, pitch, 0.0f);
			break;
		case BASS_DRUM:
			if (t == 0.0f) {
				bass_drum->play(get_volume(), false);
			}
			sample += samples;
			note_fulfilled += samples;
			break;
		case HIHAT:
			if (t == 0.0f) {
				hihat->play(get_volume(), false);
			}
			sample += samples;
			note_fulfilled += samples;
			break;
	};
}

float MML::Internal::Track::get_frequency(float start_freq)
{
	int pitch = pitches[note];
	if (pitch == -1) {
		return start_freq;
	}
	float p = note_fulfilled / (float)length_in_samples;
	int i = (int)(p * pitch_envelopes[pitch].size());
	if (i > 0) {
		start_freq = pitch_envelopes[pitch][i-1];
	}
	float stride = 1.0f / pitch_envelopes[pitch].size();
	float start = i * stride;
	float p2 = (p - start) / stride;
	return (pitch_envelopes[pitch][i] * p2) + (start_freq * (1.0f - p2));
}

float MML::Internal::Track::get_volume()
{
	while (volume_section < (int)volumes.size()-1 && sample > volumes[volume_section+1].first) {
		volume_section++;
	}
	if (volume_section >= (int)volumes.size()) {
		return volumes[volumes.size()-1].second;
	}
	else {
		float p = (sample - volumes[volume_section].first) / (float)(volumes[volume_section+1].first - volumes[volume_section].first);
		return ((1-p) * volumes[volume_section].second) + (p * volumes[volume_section+1].second);
	}
}

float MML::Internal::Track::get_dutycycle()
{
	while (dutycycle_section < (int)dutycycles.size()-1 && sample > dutycycles[dutycycle_section+1].first) {
		dutycycle_section++;
	}
	if (dutycycle_section >= (int)dutycycles.size()) {
		return dutycycles[dutycycles.size()-1].second;
	}
	else {
		float p = (sample - dutycycles[dutycycle_section].first) / (float)(dutycycles[dutycycle_section+1].first - dutycycles[dutycycle_section].first);
		return ((1-p) * dutycycles[dutycycle_section].second) + (p * dutycycles[dutycycle_section+1].second);
	}
}

std::string MML::Internal::Track::next_note(const char *text, int *pos)
{
	std::string result;
	bool done = false;
	do {
		result = token(text, pos);
		switch (result.c_str()[0]) {
			case '<':
			case '>':
				if (result.c_str()[0] == '>') {
					octave--;
				}
				else {
					octave++;
				}
				if (octave < 0) {
					octave = 0;
				}
				else if (octave > 11) {
					octave = 11;
				}
				break;
			case 'o':
				octave = atoi(result.c_str()+1);
				break;
			case 'w':
				break;
			case 'l':
				note_length = atoi(result.c_str()+1);
				break;
			case 't':
				tempo = atoi(result.c_str() + 1);
				break;
			case '@':
				if (!strncmp(result.c_str()+1, "TYPE", 4)) {
					type = (Type)atoi(result.c_str() + 5);
				}
				break;
			case 'v':
				// doesn't need to do this -- volumes set with get_volume
				break;
			case 'y':
				// doesn't need to do this -- dutycycles set with get_dutycycle
				break;
			default:
				done = true;
				break;
		}
	} while (!done);
	return result;
}

// adds waits to length
int MML::Internal::Track::notelength(const char *tok, const char *text, int *pos)
{
	char ch = *tok;
	int total = onenotelength(tok, note_length, tempo, octave, note, 'z', pitches, pitch_envelopes); // z == nothing never used
	int pos2 = *pos;
	std::string tok2;
	do {
		tok2 = token(text, &pos2);
		if (tok2.c_str()[0] == 'w') {
			total += onenotelength(tok2.c_str(), note_length, tempo, octave, note, ch, pitches, pitch_envelopes);
		}
	} while (tok2.c_str()[0] == 'w');
	return total;
}

MML::Internal::Internal(std::string filename, bool load_from_filesystem)
{
	if (load_from_filesystem == false) {
		filename = "mml/" + filename;
	}

	std::vector<std::string> tracks_s;
	std::vector< std::vector< std::pair<int, float> > > volumes;
	std::vector< std::vector< std::pair<int, float> > > dutycycles;
	std::vector< std::vector<int> > pitches;
	std::vector<int> note_lengths;
	std::vector<int> tempos;
	std::vector<int> octaves;
	std::vector<int> sample; // samples read
	std::vector<int> vol_start; // volume envelope start sample
	std::vector<int> curr_vol; // current volume envelope
	std::vector<int> duty_start; // dutycycle envelope start sample
	std::vector<int> curr_duty; // current dutycycle envelope
	std::vector<int> note;
	std::vector<int> current_pitch;
	std::vector<char> prev_ch;
	std::vector< std::vector<int> > volume_envelopes;
	std::vector< std::vector<float> > pitch_envelopes;
	std::vector< std::vector<int> > dutycycle_envelopes;
	char buf[1000];

	SDL_RWops *f;
	if (load_from_filesystem) {
		f = SDL_RWFromFile(filename.c_str(), "r");
	}
	else {
		f = open_file(filename);
	}

	while (SDL_fgets(f, buf, 1000)) {
		int pos = 0;
		std::string tok = token(buf, &pos);
		if (tok.c_str()[0] >= 'A' && tok.c_str()[0] <= 'Z') {
			int track = tok.c_str()[0] - 'A';
			while ((int)tracks_s.size() < track+1) {
				tracks_s.push_back("");
				volumes.push_back(std::vector< std::pair<int, float> >());
				volumes[volumes.size()-1].push_back(std::pair<int, float>(0, 1.0f));
				dutycycles.push_back(std::vector< std::pair<int, float> >());
				dutycycles[dutycycles.size()-1].push_back(std::pair<int, float>(0, 0.5f));
				pitches.push_back(std::vector<int>());
				note_lengths.push_back(4);
				tempos.push_back(120);
				octaves.push_back(4);
				sample.push_back(0);
				vol_start.push_back(0);
				curr_vol.push_back(-1);
				duty_start.push_back(0);
				curr_duty.push_back(-1);
				note.push_back(0);
				current_pitch.push_back(-1);
				prev_ch.push_back('z');
			}
			while (isspace(buf[strlen(buf) - 1]))
				buf[strlen(buf) - 1] = 0;
			tracks_s[track] += std::string(buf + pos);
			/* Pre-process for envelopes */
			tok = token(buf, &pos);
			while (tok.c_str()[0]) {
				if (tok.c_str()[0] == '@') {
					if (tok.c_str()[1] == 'V') {
						if (curr_vol[track] == -1) {
							vol_start[track] = sample[track];
							curr_vol[track] = atoi(tok.c_str() + 2);
						}
						else {
							int length = sample[track] - vol_start[track];
							if ((int)volume_envelopes.size() > curr_vol[track]) {
								int sz = volume_envelopes[curr_vol[track]].size();
								int stride = sz <= 1 ? length : length / (sz-1);
								float start_vol;
								if (volumes[track].size() > 0) {
									start_vol = volumes[track][volumes.size()-1].second;
								}
								else {
									start_vol = 1.0f;
								}
								for (int i = 0; i < sz; i++) {
									volumes[track].push_back(std::pair<int, float>(
										vol_start[track] + stride * i,
										volume_envelopes[curr_vol[track]][i]/255.0f
									));
								}
								volumes[track].push_back(std::pair<int, float>(
									vol_start[track] + length,
									start_vol
								));
							}
							curr_vol[track] = -1;
						}
					}
					else if (tok.c_str()[1] == 'P') {
						current_pitch[track] = atoi(tok.c_str() + 2);
						if (current_pitch[track] >= (int)pitch_envelopes.size()) {
							current_pitch[track] = -1;
						}
					}
					else if (tok.c_str()[1] == 'D') {
						if (curr_duty[track] == -1) {
							duty_start[track] = sample[track];
							curr_duty[track] = atoi(tok.c_str() + 2);
						}
						else {
							if (dutycycles[track].size() > 0) {
								dutycycles[track].push_back(std::pair<int, float>(
									sample[track],
									dutycycles[track][dutycycles[track].size()-1].second
								));
							}
							int length = sample[track] - duty_start[track];
							if ((int)dutycycle_envelopes.size() > curr_duty[track]) {
								int sz = dutycycle_envelopes[curr_duty[track]].size();
								int stride = length / (sz-1);
								for (int i = 0; i < sz; i++) {
									dutycycles[track].push_back(std::pair<int, float>(
										duty_start[track] + stride * i,
										dutycycle_envelopes[curr_duty[track]][i]/255.0f
									));
								}
							}
							curr_duty[track] = -1;
						}
					}
				}
				else {
					if ((tok.c_str()[0] >= 'a' && tok.c_str()[0] <= 'g') || tok.c_str()[0] == 'r' || tok.c_str()[0] == 'w') {
						if (tok.c_str()[0] >= 'a' && tok.c_str()[0] <= 'g') {
							prev_ch[track] = tok.c_str()[0];
						}
						pitches[track].push_back(current_pitch[track]);
						sample[track] += onenotelength(tok.c_str(), note_lengths[track], tempos[track], octaves[track], note[track], prev_ch[track], pitches[track], pitch_envelopes);
						current_pitch[track] = -1;
						note[track]++;
					}
					else if (tok.c_str()[0] == 'v') {
						float vol = atoi(tok.c_str() + 1) / 255.0f;
						volumes[track].push_back(std::pair<int, float>(sample[track], vol));
					}
					else if (tok.c_str()[0] == 'y') {
						float duty = atoi(tok.c_str() + 1) / 255.0f;
						if (dutycycles[track].size() > 0) {
							dutycycles[track].push_back(std::pair<int, float>(
								sample[track],
								dutycycles[track][dutycycles[track].size()-1].second
							));
						}
						dutycycles[track].push_back(std::pair<int, float>(sample[track], duty));
					}
					else if (tok.c_str()[0] == 'l') {
						note_lengths[track] = atoi(tok.c_str() + 1);
					}
					else if (tok.c_str()[0] == 't') {
						tempos[track] = atoi(tok.c_str() + 1);
					}
					else if (tok.c_str()[0] == 'o') {
						octaves[track] = atoi(tok.c_str() + 1);
					}
					else if (tok.c_str()[0] == '>') {
						octaves[track]--;
						if (octaves[track] < 0) {
							octaves[track] = 0;
						}
					}
					else if (tok.c_str()[0] == '<') {
						octaves[track]++;
						if (octaves[track] > 11) {
							octaves[track] = 11;
						}
					}
				}
				tok = token(buf, &pos);
			}
		}
		else if (tok.c_str()[0] == '@') {
			if (tok.c_str()[1] == 'V') {
				int num = atoi(tok.c_str() + 2);
				std::vector<int> envelope;
				while (tok.c_str()[0] != 0) {
					tok = token(buf, &pos);
					if (isdigit(tok.c_str()[0])) {
						envelope.push_back(atoi(tok.c_str()));
					}
				}
				while ((int)volume_envelopes.size() <= num) {
					volume_envelopes.push_back(std::vector<int>());
				}
				volume_envelopes[num] = envelope;
			}
			else if (tok.c_str()[1] == 'P') {
				int num = atoi(tok.c_str() + 2);
				std::vector<float> envelope;
				while (tok.c_str()[0] != 0) {
					tok = token(buf, &pos);
					if (tok.c_str()[0] >= 'a' && tok.c_str()[0] <= 'g') {
						int index = indexes[tok.c_str()[0] - 'a'];
						int offset = 1;
						if (tok.c_str()[1] == '+') {
							index++;
							offset++;
						}
						else if (tok.c_str()[1] == '-') {
							index--;
							offset++;
						}
						int octave = atoi(tok.c_str() + offset);
						if (octave < 0) {
							octave = 0;
						}
						else if (octave > 11) {
							octave = 11;
						}
						float pitch = note_pitches[index][octave];
						envelope.push_back(pitch);
					}
				}
				while ((int)pitch_envelopes.size() <= num) {
					pitch_envelopes.push_back(std::vector<float>());
				}
				pitch_envelopes[num] = envelope;
			}
			else if (tok.c_str()[1] == 'D') {
				int num = atoi(tok.c_str() + 2);
				std::vector<int> envelope;
				while (tok.c_str()[0] != 0) {
					tok = token(buf, &pos);
					if (isdigit(tok.c_str()[0])) {
						envelope.push_back(atoi(tok.c_str()));
					}
				}
				while ((int)dutycycle_envelopes.size() <= num) {
					dutycycle_envelopes.push_back(std::vector<int>());
				}
				dutycycle_envelopes[num] = envelope;
			}
		}
	}
	SDL_RWclose(f);

	int longest = 0;

	for (size_t i = 0; i < tracks_s.size(); i++) {
		if (sample[i] > longest) {
			longest = sample[i];
		}

		// Volumes and duty cycles must have an explicit ending

		volumes[i].push_back(std::pair<int, float>(
			sample[i],
			volumes[i][volumes[i].size() - 1].second
			));

		dutycycles[i].push_back(std::pair<int, float>(
			sample[i],
			dutycycles[i][dutycycles[i].size() - 1].second
			));
	}

	for (size_t i = 0; i < tracks_s.size(); i++) {
		tracks.push_back(new Track(MML::Internal::Track::PULSE, tracks_s[i], volumes[i], pitches[i], pitch_envelopes, dutycycles[i], longest-sample[i]));
	}
}

MML::Internal::~Internal()
{
	for (size_t i =  0; i < tracks.size(); i++) {
		delete tracks[i];
	}
}

void MML::mix(int32_t *buf, int stream_length)
{
	Uint8 *tmp = new Uint8[stream_length];

	for (size_t i = 0; i < loaded_mml.size(); i++) {
		std::vector<Internal::Track *> &tracks = loaded_mml[i]->tracks;
		for (size_t track = 0; track < tracks.size(); track++) {
			if (tracks[track]->is_playing()) {
				if (tracks[track]->update((short *)tmp, stream_length/2)) {
					for (int i = 0; i < stream_length / 2; i++) {
						int32_t src1 = *((int32_t *)buf + i);
						int16_t src2 = int16_t(*((int16_t *)tmp + i) * 0.25f);
						int32_t result = src1 + src2;
						*((int32_t *)buf + i) = result;
					}
				}
			}
		}
	}

	delete[] tmp;
}

MML::MML(std::string filename, bool load_from_filesystem) :
	name(filename)
{
	internal = new MML::Internal(filename, load_from_filesystem);
	loaded_mml.push_back(internal);
}

MML::~MML()
{
	for (size_t i = 0; i < loaded_mml.size(); i++) {
		if (loaded_mml[i] == internal) {
			loaded_mml.erase(loaded_mml.begin()+i);
			break;
		}
	}

	delete internal;
}

void MML::play(bool loop)
{
	if (noo.mute) {
		return;
	}

	std::vector<MML::Internal::Track *> &tracks = internal->tracks;

	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->play(loop);
	}
}

void MML::stop()
{
	std::vector<MML::Internal::Track *> &tracks = internal->tracks;

	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->stop();
	}
}

std::string MML::get_name()
{
	return name;
}

bool MML::is_done()
{
	for (size_t i = 0; i < internal->tracks.size(); i++) {
		if (internal->tracks[i]->is_done() == false) {
			return false;
		}
	}

	return true;
}
