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

	bool open(std::string const& _device = std::string());
	bool isOpen() const { return m_cdrom && m_paranoia; }
	unsigned tracks() const;
	void rip(unsigned _track, std::function<void(unsigned, unsigned, int16_t const*, size_t)> const& _f, int _flags = -1);
	void close();

private:
	cdrom_drive_s* m_cdrom;
	cdrom_paranoia_s* m_paranoia;
};

