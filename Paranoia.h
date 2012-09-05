#pragma once

#include <map>
#include <string>
#include <functional>
#include <cstdint>

struct cdrom_drive_s;
struct cdrom_paranoia_s;

class Paranoia
{
public:
	Paranoia();
	~Paranoia();

	static size_t frameLength();
	bool open(std::string const& _device = std::string());
	bool isOpen() const { return m_cdrom && m_paranoia; }
	unsigned tracks() const;
	size_t trackLength(unsigned _t) const;	// in 44100 Hz samples.
	void rip(unsigned _track, std::function<bool(unsigned, unsigned, int16_t const*)> const& _f, int _flags = -1);
	void close();

	static int defaultFlags();
	static std::map<std::string, std::string> devices();

private:
	cdrom_drive_s* m_cdrom;
	cdrom_paranoia_s* m_paranoia;
};

