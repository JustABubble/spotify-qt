#include "lib/lyrics.hpp"

lib::lyrics::lyrics(const lib::http_client &http_client)
	: http(http_client)
{
}

void lib::lyrics::get(const spt::track &track, lib::result<std::string> &callback)
{
	auto artist = lib::strings::capitalize(format(track.artists.front().name));
	auto name = lib::strings::to_lower(format(track.name));
	auto url = lib::fmt::format("https://genius.com/{}-{}-lyrics",
		artist, name);

	http.get(url, lib::headers(), [url, callback](const std::string &body)
	{
		if (body.empty())
		{
			callback(false, "No response");
			return;
		}

		if (lib::strings::contains(body, "Burrr! | Genius"))
		{
			lib::log::warn("No lyrics found from: {}", url);
			callback(false, "No results");
			return;
		}

		auto lyrics = get_from_lyrics(body);
		if (!lyrics.empty())
		{
			callback(true, lyrics);
			return;
		}

		lyrics = get_from_lyrics_root(body);
		if (!lyrics.empty())
		{
			callback(true, lyrics);
			return;
		}

		lib::log::warn("No lyrics parsed from: {}", url);
		callback(false, "No results");
	});
}

auto lib::lyrics::format(const std::string &str) -> std::string
{
	auto val = lib::strings::replace_all(str, ' ', '-');
	val.erase(std::remove_if(val.begin(), val.end(), [](unsigned char c) -> bool
	{
		return c != '-' && std::isalpha(c) == 0;
	}), val.end());
	return val;
}

auto lib::lyrics::get_from_lyrics(const std::string &lyrics) -> std::string
{
	std::string start_str = "<div class=\"lyrics\">";
	auto start = lyrics.find(start_str);
	auto end = lyrics.find("</div>", start);

	if (start == std::string::npos
		|| end == std::string::npos)
	{
		return std::string();
	}

	return lyrics.substr(start + start_str.size(), end);
}

auto lib::lyrics::get_from_lyrics_root(const std::string &/*lyrics*/) -> std::string
{
	return std::string();
}