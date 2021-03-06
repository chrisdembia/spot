#include "file_reporter.h"

#include <fstream>

#include "xo/system/system_tools.h"
#include "xo/filesystem/filesystem.h"
#include "xo/container/container_tools.h"

#include "optimizer.h"
#include "xo/system/log.h"

namespace spot
{
	file_reporter::file_reporter( const path& root_folder ) : root_( root_folder ), last_output_step( -1 )
	{}

	void file_reporter::on_start( const optimizer& opt )
	{
		xo::create_directories( root_ );

		// setup history.txt
		history_ = std::ofstream( ( root_ / "history.txt" ).string() );
		history_ << "Step\tBest\tMedian\tPredicted\tProgress" << std::endl;
	}

	void file_reporter::on_stop( const optimizer& opt, const stop_condition& s )
	{}

	void file_reporter::on_pre_evaluate_population( const optimizer& opt, const search_point_vec& pop )
	{
		if ( output_temp_files )
		{
			// create temp files for debugging purposes
			for ( index_t i = 0; i < pop.size(); ++i )
			{
				path p = root_ / stringf( "%04d_%02d.tmp", opt.current_step(), i );
				std::ofstream( p.str() ) << pop[ i ];
			}
		}
	}

	void file_reporter::on_post_evaluate_population( const optimizer& opt, const search_point_vec& pop, const fitness_vec_t& fitnesses, bool new_best )
	{
		if ( output_temp_files )
		{
			// remove temp files
			for ( index_t i = 0; i < pop.size(); ++i )
			{
				path p = root_ / stringf( "%04d_%02d.tmp", opt.current_step(), i );
				remove( p );
			}
		}

		if ( opt.current_step() - last_output_step >= max_steps_without_file_output )
			write_par_file( opt, false );
	}

	void file_reporter::on_new_best( const optimizer& opt, const search_point& sp, fitness_t best )
	{
		write_par_file( opt, true );
	}

	void file_reporter::on_post_step( const optimizer& opt )
	{
		// update history
		auto cur_trend = opt.fitness_trend();
		history_ << opt.current_step() << "\t" << opt.current_step_best_fitness() << "\t" << xo::median( opt.current_step_fitnesses() );

		if ( opt.fitness_tracking_window_size() > 0 )
			history_ << "\t" << opt.predicted_fitness( opt.fitness_tracking_window_size() ) << "\t" << opt.progress();

		history_ << "\n";
		if ( opt.current_step() % 10 == 9 ) // flush every 10 entries
			history_.flush();
	}

	void file_reporter::write_par_file( const optimizer& opt, bool try_cleanup )
	{
		const fitness_t replim = 999999.999;
		auto best = opt.current_step_best_fitness();
		objective_info updated_info = opt.make_updated_objective_info();
		auto sp = search_point( updated_info, opt.current_step_best_point().values() );
		auto avg = xo::median( opt.current_step_fitnesses() );
		path filename = root_ / xo::stringf( "%04d_%.3f_%.3f.par", opt.current_step(), xo::clamped( avg, -replim, replim ), xo::clamped( best, -replim, replim ) );
		std::ofstream str( filename.str() );
		str << sp;

		if ( try_cleanup )
		{
			recent_files.push_back( std::make_pair( filename, best ) );
			xo_assert( recent_files.size() <= 3 );
			if ( recent_files.size() == 3 )
			{
				// see if we should delete the second last file
				auto& f1 = recent_files[ 0 ].second;
				auto& f2 = recent_files[ 1 ].second;
				auto& f3 = recent_files[ 2 ].second;

				double imp1 = ( f2 - f1 ) / abs( f1 );
				double imp2 = ( f3 - f2 ) / abs( f2 );
				if ( opt.info().minimize() )
					imp1 = -imp1, imp2 = -imp2;

				if ( imp1 < min_improvement_for_file_output && imp2 < min_improvement_for_file_output )
				{
					//xo::log::info( "Cleaning up file ", recent_files[ 1 ].first );
					xo::remove( recent_files[ 1 ].first );
					recent_files[ 1 ] = recent_files[ 2 ];
					recent_files.pop_back();
				}
				else recent_files.pop_front();
			}
		}

		last_output_step = opt.current_step();
	}
}
