#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "candybox/FSM.hpp"


class player : public candybox::fsm<player>
{
	friend class candybox::fsm<player>; // base class needs access to transition_table

	std::string cd_title;
	bool autoplay = false;

public:
	enum states
	{
		Stopped,
		Open,
		Empty,
		Playing,
		Paused
	};

	player(state_type init_state = Empty) : fsm(init_state) { }

	void set_autoplay(bool f) { autoplay = f; }

	bool is_autoplay() const { return autoplay; }

	const std::string& get_cd_title() const { return cd_title; }

	struct play
	{
	};
	struct open_close
	{
	};
	struct cd_detected
	{
		std::string title;
	};
	struct stop
	{
	};
	struct pause
	{
	};

private:
	// guards
	bool is_autoplay(const cd_detected&) const { return autoplay; }
	bool is_bad_cd(const cd_detected& cd) const { return cd.title.empty(); }

	// actions
	void start_playback(const play&);
	void start_autoplay(const cd_detected& cd);
	void open_drawer(const open_close&);
	void open_drawer(const cd_detected& cd);
	void close_drawer(const open_close&);
	void store_cd_info(const cd_detected& cd);
	void stop_playback(const stop&);
	void pause_playback(const pause&);
	void stop_and_open(const open_close&);
	void resume_playback(const play&);

private:
	using m = player; // for brevity

	using transition_table = table<
	    // Start, Event, Target, Action, Guard (optional)
	    mem_fn_row<Stopped, play, Playing, &m::start_playback>,
	    mem_fn_row<Stopped, open_close, Open, &m::open_drawer>,
	    mem_fn_row<Open, open_close, Empty, &m::close_drawer>,
	    mem_fn_row<Empty, open_close, Open, &m::open_drawer>,
	    mem_fn_row<Empty, cd_detected, Open, &m::open_drawer, &m::is_bad_cd>,
	    mem_fn_row<Empty, cd_detected, Playing, &m::start_autoplay, &m::is_autoplay>,
	    mem_fn_row<Empty, cd_detected, Stopped, &m::store_cd_info /* fallback */>,
	    mem_fn_row<Playing, stop, Stopped, &m::stop_playback>,
	    mem_fn_row<Playing, pause, Paused, &m::pause_playback>,
	    mem_fn_row<Playing, open_close, Open, &m::stop_and_open>,
	    mem_fn_row<Paused, play, Playing, &m::resume_playback>,
	    mem_fn_row<Paused, stop, Stopped, &m::stop_playback>,
	    mem_fn_row<Paused, open_close, Open, &m::stop_and_open> >;
};

void
player::start_playback(const play&)
{
	std::cout << "Starting playback\n";
}

void
player::start_autoplay(const cd_detected& cd)
{
	std::cout << "Starting playback of '" << cd.title << "'\n";
	cd_title = cd.title;
}

void
player::open_drawer(const open_close&)
{
	std::cout << "Opening drawer\n";
	cd_title.clear();
}

void
player::open_drawer(const cd_detected&)
{
	std::cout << "Ejecting bad CD\n";
	cd_title.clear();
}

void
player::close_drawer(const open_close&)
{
	std::cout << "Closing drawer\n";
}

void
player::store_cd_info(const cd_detected& cd)
{
	std::cout << "Detected CD '" << cd.title << "'\n";
	cd_title = cd.title;
}

void
player::stop_playback(const stop&)
{
	std::cout << "Stopping playback\n";
}

void
player::pause_playback(const pause&)
{
	std::cout << "Pausing playback\n";
}

void
player::stop_and_open(const open_close&)
{
	std::cout << "Stopping and opening drawer\n";
	cd_title.clear();
}

void
player::resume_playback(const play&)
{
	std::cout << "Resuming playback\n";
}

// ----------------------------------------------------------------

TEST_CASE("Test player", "[test]")
{
	player p;
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(!p.is_autoplay());
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Open);
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Empty);
	p.process_event(player::cd_detected{"louie, louie"});
	REQUIRE(p.current_state() == player::Stopped);
	REQUIRE(p.get_cd_title() == "louie, louie");
	p.process_event(player::play());
	REQUIRE(p.current_state() == player::Playing);
	p.process_event(player::pause());
	REQUIRE(p.current_state() == player::Paused);
	p.process_event(player::play());
	REQUIRE(p.current_state() == player::Playing);
	p.process_event(player::stop());
	REQUIRE(p.current_state() == player::Stopped);
	p.process_event(player::play());
	REQUIRE(p.current_state() == player::Playing);
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Open);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::play());
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(p.get_cd_title().empty());
}

TEST_CASE("Test bad cd", "[bad_cd]")
{
	player p;
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(!p.is_autoplay());
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Open);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::cd_detected{});
	REQUIRE(p.current_state() == player::Open);
	REQUIRE(p.get_cd_title().empty());
}

TEST_CASE("Test auto play", "[auto_play]")
{
	player p;
	p.set_autoplay(true);
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(p.is_autoplay());
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Open);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::open_close());
	REQUIRE(p.current_state() == player::Empty);
	REQUIRE(p.get_cd_title().empty());
	p.process_event(player::cd_detected{"louie, louie"});
	REQUIRE(p.current_state() == player::Playing);
	REQUIRE(p.get_cd_title() == "louie, louie");
}



int
main(int argc, char* argv[])
{
	int result = Catch::Session().run(argc, argv);

	return result;
}
