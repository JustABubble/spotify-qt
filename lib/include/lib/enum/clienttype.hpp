#pragma once

namespace lib
{
	/**
	 * Supported Spotify clients
	 */
	enum class [[deprecated("only librespot is supported")]] client_type: unsigned char
	{
		/**
		 * No client
		 */
		none,

		/**
		 * Spotifyd
		 * @see https://github.com/Spotifyd/spotifyd
		 */
		spotifyd,

		/**
		 * librespot
		 * @see https://github.com/librespot-org/librespot
		 */
		librespot
	};
}
