#include <stdint.h>

#define pa_operation_unref pa_operation_unref_dylibloader_orig_pulse
#define pa_operation_get_state pa_operation_get_state_dylibloader_orig_pulse
#define pa_context_new pa_context_new_dylibloader_orig_pulse
#define pa_context_unref pa_context_unref_dylibloader_orig_pulse
#define pa_context_set_state_callback pa_context_set_state_callback_dylibloader_orig_pulse
#define pa_context_get_state pa_context_get_state_dylibloader_orig_pulse
#define pa_context_connect pa_context_connect_dylibloader_orig_pulse
#define pa_context_disconnect pa_context_disconnect_dylibloader_orig_pulse
#define pa_context_get_sink_info_list pa_context_get_sink_info_list_dylibloader_orig_pulse
#define pa_context_get_source_info_list pa_context_get_source_info_list_dylibloader_orig_pulse
#define pa_context_load_module pa_context_load_module_dylibloader_orig_pulse
#define pa_context_unload_module pa_context_unload_module_dylibloader_orig_pulse
#define pa_strerror pa_strerror_dylibloader_orig_pulse
#define pa_mainloop_new pa_mainloop_new_dylibloader_orig_pulse
#define pa_mainloop_free pa_mainloop_free_dylibloader_orig_pulse
#define pa_mainloop_iterate pa_mainloop_iterate_dylibloader_orig_pulse
#define pa_mainloop_get_api pa_mainloop_get_api_dylibloader_orig_pulse
#define pa_simple_new pa_simple_new_dylibloader_orig_pulse
#define pa_simple_free pa_simple_free_dylibloader_orig_pulse
#define pa_simple_write pa_simple_write_dylibloader_orig_pulse
#define pa_simple_drain pa_simple_drain_dylibloader_orig_pulse
#define pa_simple_read pa_simple_read_dylibloader_orig_pulse
#define pa_simple_get_latency pa_simple_get_latency_dylibloader_orig_pulse
#define pa_simple_flush pa_simple_flush_dylibloader_orig_pulse

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#undef pa_get_library_version
#undef pa_bytes_per_second
#undef pa_frame_size
#undef pa_sample_size
#undef pa_sample_size_of_format
#undef pa_bytes_to_usec
#undef pa_usec_to_bytes
#undef pa_sample_spec_init
#undef pa_sample_format_valid
#undef pa_sample_rate_valid
#undef pa_channels_valid
#undef pa_sample_spec_valid
#undef pa_sample_spec_equal
#undef pa_sample_format_to_string
#undef pa_parse_sample_format
#undef pa_sample_spec_snprint
#undef pa_bytes_snprint
#undef pa_sample_format_is_le
#undef pa_sample_format_is_be
#undef pa_direction_valid
#undef pa_direction_to_string
#undef pa_mainloop_api_once
#undef pa_proplist_new
#undef pa_proplist_free
#undef pa_proplist_key_valid
#undef pa_proplist_sets
#undef pa_proplist_setp
#undef pa_proplist_setf
#undef pa_proplist_set
#undef pa_proplist_gets
#undef pa_proplist_get
#undef pa_proplist_update
#undef pa_proplist_unset
#undef pa_proplist_unset_many
#undef pa_proplist_iterate
#undef pa_proplist_to_string
#undef pa_proplist_to_string_sep
#undef pa_proplist_from_string
#undef pa_proplist_contains
#undef pa_proplist_clear
#undef pa_proplist_copy
#undef pa_proplist_size
#undef pa_proplist_isempty
#undef pa_proplist_equal
#undef pa_channel_map_init
#undef pa_channel_map_init_mono
#undef pa_channel_map_init_stereo
#undef pa_channel_map_init_auto
#undef pa_channel_map_init_extend
#undef pa_channel_position_to_string
#undef pa_channel_position_from_string
#undef pa_channel_position_to_pretty_string
#undef pa_channel_map_snprint
#undef pa_channel_map_parse
#undef pa_channel_map_equal
#undef pa_channel_map_valid
#undef pa_channel_map_compatible
#undef pa_channel_map_superset
#undef pa_channel_map_can_balance
#undef pa_channel_map_can_fade
#undef pa_channel_map_can_lfe_balance
#undef pa_channel_map_to_name
#undef pa_channel_map_to_pretty_name
#undef pa_channel_map_has_position
#undef pa_channel_map_mask
#undef pa_encoding_to_string
#undef pa_encoding_from_string
#undef pa_format_info_new
#undef pa_format_info_copy
#undef pa_format_info_free
#undef pa_format_info_valid
#undef pa_format_info_is_pcm
#undef pa_format_info_is_compatible
#undef pa_format_info_snprint
#undef pa_format_info_from_string
#undef pa_format_info_from_sample_spec
#undef pa_format_info_to_sample_spec
#undef pa_format_info_get_prop_type
#undef pa_format_info_get_prop_int
#undef pa_format_info_get_prop_int_range
#undef pa_format_info_get_prop_int_array
#undef pa_format_info_get_prop_string
#undef pa_format_info_get_prop_string_array
#undef pa_format_info_free_string_array
#undef pa_format_info_get_sample_format
#undef pa_format_info_get_rate
#undef pa_format_info_get_channels
#undef pa_format_info_get_channel_map
#undef pa_format_info_set_prop_int
#undef pa_format_info_set_prop_int_array
#undef pa_format_info_set_prop_int_range
#undef pa_format_info_set_prop_string
#undef pa_format_info_set_prop_string_array
#undef pa_format_info_set_sample_format
#undef pa_format_info_set_rate
#undef pa_format_info_set_channels
#undef pa_format_info_set_channel_map
#undef pa_operation_ref
#undef pa_operation_unref
#undef pa_operation_cancel
#undef pa_operation_get_state
#undef pa_operation_set_state_callback
#undef pa_context_new
#undef pa_context_new_with_proplist
#undef pa_context_unref
#undef pa_context_ref
#undef pa_context_set_state_callback
#undef pa_context_set_event_callback
#undef pa_context_errno
#undef pa_context_is_pending
#undef pa_context_get_state
#undef pa_context_connect
#undef pa_context_disconnect
#undef pa_context_drain
#undef pa_context_exit_daemon
#undef pa_context_set_default_sink
#undef pa_context_set_default_source
#undef pa_context_is_local
#undef pa_context_set_name
#undef pa_context_get_server
#undef pa_context_get_protocol_version
#undef pa_context_get_server_protocol_version
#undef pa_context_proplist_update
#undef pa_context_proplist_remove
#undef pa_context_get_index
#undef pa_context_rttime_new
#undef pa_context_rttime_restart
#undef pa_context_get_tile_size
#undef pa_context_load_cookie_from_file
#undef pa_cvolume_equal
#undef pa_cvolume_init
#undef pa_cvolume_set
#undef pa_cvolume_snprint
#undef pa_sw_cvolume_snprint_dB
#undef pa_cvolume_snprint_verbose
#undef pa_volume_snprint
#undef pa_sw_volume_snprint_dB
#undef pa_volume_snprint_verbose
#undef pa_cvolume_avg
#undef pa_cvolume_avg_mask
#undef pa_cvolume_max
#undef pa_cvolume_max_mask
#undef pa_cvolume_min
#undef pa_cvolume_min_mask
#undef pa_cvolume_valid
#undef pa_cvolume_channels_equal_to
#undef pa_sw_volume_multiply
#undef pa_sw_cvolume_multiply
#undef pa_sw_cvolume_multiply_scalar
#undef pa_sw_volume_divide
#undef pa_sw_cvolume_divide
#undef pa_sw_cvolume_divide_scalar
#undef pa_sw_volume_from_dB
#undef pa_sw_volume_to_dB
#undef pa_sw_volume_from_linear
#undef pa_sw_volume_to_linear
#undef pa_cvolume_remap
#undef pa_cvolume_compatible
#undef pa_cvolume_compatible_with_channel_map
#undef pa_cvolume_get_balance
#undef pa_cvolume_set_balance
#undef pa_cvolume_get_fade
#undef pa_cvolume_set_fade
#undef pa_cvolume_get_lfe_balance
#undef pa_cvolume_set_lfe_balance
#undef pa_cvolume_scale
#undef pa_cvolume_scale_mask
#undef pa_cvolume_set_position
#undef pa_cvolume_get_position
#undef pa_cvolume_merge
#undef pa_cvolume_inc_clamp
#undef pa_cvolume_inc
#undef pa_cvolume_dec
#undef pa_stream_new
#undef pa_stream_new_with_proplist
#undef pa_stream_new_extended
#undef pa_stream_unref
#undef pa_stream_ref
#undef pa_stream_get_state
#undef pa_stream_get_context
#undef pa_stream_get_index
#undef pa_stream_get_device_index
#undef pa_stream_get_device_name
#undef pa_stream_is_suspended
#undef pa_stream_is_corked
#undef pa_stream_connect_playback
#undef pa_stream_connect_record
#undef pa_stream_disconnect
#undef pa_stream_begin_write
#undef pa_stream_cancel_write
#undef pa_stream_write
#undef pa_stream_write_ext_free
#undef pa_stream_peek
#undef pa_stream_drop
#undef pa_stream_writable_size
#undef pa_stream_readable_size
#undef pa_stream_drain
#undef pa_stream_update_timing_info
#undef pa_stream_set_state_callback
#undef pa_stream_set_write_callback
#undef pa_stream_set_read_callback
#undef pa_stream_set_overflow_callback
#undef pa_stream_get_underflow_index
#undef pa_stream_set_underflow_callback
#undef pa_stream_set_started_callback
#undef pa_stream_set_latency_update_callback
#undef pa_stream_set_moved_callback
#undef pa_stream_set_suspended_callback
#undef pa_stream_set_event_callback
#undef pa_stream_set_buffer_attr_callback
#undef pa_stream_cork
#undef pa_stream_flush
#undef pa_stream_prebuf
#undef pa_stream_trigger
#undef pa_stream_set_name
#undef pa_stream_get_time
#undef pa_stream_get_latency
#undef pa_stream_get_timing_info
#undef pa_stream_get_sample_spec
#undef pa_stream_get_channel_map
#undef pa_stream_get_format_info
#undef pa_stream_get_buffer_attr
#undef pa_stream_set_buffer_attr
#undef pa_stream_update_sample_rate
#undef pa_stream_proplist_update
#undef pa_stream_proplist_remove
#undef pa_stream_set_monitor_stream
#undef pa_stream_get_monitor_stream
#undef pa_context_get_sink_info_by_name
#undef pa_context_get_sink_info_by_index
#undef pa_context_get_sink_info_list
#undef pa_context_set_sink_volume_by_index
#undef pa_context_set_sink_volume_by_name
#undef pa_context_set_sink_mute_by_index
#undef pa_context_set_sink_mute_by_name
#undef pa_context_suspend_sink_by_name
#undef pa_context_suspend_sink_by_index
#undef pa_context_set_sink_port_by_index
#undef pa_context_set_sink_port_by_name
#undef pa_context_get_source_info_by_name
#undef pa_context_get_source_info_by_index
#undef pa_context_get_source_info_list
#undef pa_context_set_source_volume_by_index
#undef pa_context_set_source_volume_by_name
#undef pa_context_set_source_mute_by_index
#undef pa_context_set_source_mute_by_name
#undef pa_context_suspend_source_by_name
#undef pa_context_suspend_source_by_index
#undef pa_context_set_source_port_by_index
#undef pa_context_set_source_port_by_name
#undef pa_context_get_server_info
#undef pa_context_get_module_info
#undef pa_context_get_module_info_list
#undef pa_context_load_module
#undef pa_context_unload_module
#undef pa_context_get_client_info
#undef pa_context_get_client_info_list
#undef pa_context_kill_client
#undef pa_context_get_card_info_by_index
#undef pa_context_get_card_info_by_name
#undef pa_context_get_card_info_list
#undef pa_context_set_card_profile_by_index
#undef pa_context_set_card_profile_by_name
#undef pa_context_set_port_latency_offset
#undef pa_context_get_sink_input_info
#undef pa_context_get_sink_input_info_list
#undef pa_context_move_sink_input_by_name
#undef pa_context_move_sink_input_by_index
#undef pa_context_set_sink_input_volume
#undef pa_context_set_sink_input_mute
#undef pa_context_kill_sink_input
#undef pa_context_get_source_output_info
#undef pa_context_get_source_output_info_list
#undef pa_context_move_source_output_by_name
#undef pa_context_move_source_output_by_index
#undef pa_context_set_source_output_volume
#undef pa_context_set_source_output_mute
#undef pa_context_kill_source_output
#undef pa_context_stat
#undef pa_context_get_sample_info_by_name
#undef pa_context_get_sample_info_by_index
#undef pa_context_get_sample_info_list
#undef pa_context_get_autoload_info_by_name
#undef pa_context_get_autoload_info_by_index
#undef pa_context_get_autoload_info_list
#undef pa_context_add_autoload
#undef pa_context_remove_autoload_by_name
#undef pa_context_remove_autoload_by_index
#undef pa_context_subscribe
#undef pa_context_set_subscribe_callback
#undef pa_stream_connect_upload
#undef pa_stream_finish_upload
#undef pa_context_remove_sample
#undef pa_context_play_sample
#undef pa_context_play_sample_with_proplist
#undef pa_strerror
#undef pa_xmalloc
#undef pa_xmalloc0
#undef pa_xrealloc
#undef pa_xfree
#undef pa_xstrdup
#undef pa_xstrndup
#undef pa_xmemdup
#undef pa_utf8_valid
#undef pa_ascii_valid
#undef pa_utf8_filter
#undef pa_ascii_filter
#undef pa_utf8_to_locale
#undef pa_locale_to_utf8
#undef pa_threaded_mainloop_new
#undef pa_threaded_mainloop_free
#undef pa_threaded_mainloop_start
#undef pa_threaded_mainloop_stop
#undef pa_threaded_mainloop_lock
#undef pa_threaded_mainloop_unlock
#undef pa_threaded_mainloop_wait
#undef pa_threaded_mainloop_signal
#undef pa_threaded_mainloop_accept
#undef pa_threaded_mainloop_get_retval
#undef pa_threaded_mainloop_get_api
#undef pa_threaded_mainloop_in_thread
#undef pa_threaded_mainloop_set_name
#undef pa_threaded_mainloop_once_unlocked
#undef pa_mainloop_new
#undef pa_mainloop_free
#undef pa_mainloop_prepare
#undef pa_mainloop_poll
#undef pa_mainloop_dispatch
#undef pa_mainloop_get_retval
#undef pa_mainloop_iterate
#undef pa_mainloop_run
#undef pa_mainloop_get_api
#undef pa_mainloop_quit
#undef pa_mainloop_wakeup
#undef pa_mainloop_set_poll_func
#undef pa_signal_init
#undef pa_signal_done
#undef pa_signal_new
#undef pa_signal_free
#undef pa_signal_set_destroy
#undef pa_get_user_name
#undef pa_get_host_name
#undef pa_get_fqdn
#undef pa_get_home_dir
#undef pa_get_binary_name
#undef pa_path_get_filename
#undef pa_msleep
#undef pa_thread_make_realtime
#undef pa_gettimeofday
#undef pa_timeval_diff
#undef pa_timeval_cmp
#undef pa_timeval_age
#undef pa_timeval_add
#undef pa_timeval_sub
#undef pa_timeval_store
#undef pa_timeval_load
#undef pa_rtclock_now
#undef pa_simple_new
#undef pa_simple_free
#undef pa_simple_write
#undef pa_simple_drain
#undef pa_simple_read
#undef pa_simple_get_latency
#undef pa_simple_flush

#include <dlfcn.h>
#include <stdio.h>

void (*pa_operation_unref_dylibloader_wrapper_pulse)( pa_operation*);
pa_operation_state_t (*pa_operation_get_state_dylibloader_wrapper_pulse)(const pa_operation*);
pa_context* (*pa_context_new_dylibloader_wrapper_pulse)( pa_mainloop_api*,const char*);
void (*pa_context_unref_dylibloader_wrapper_pulse)( pa_context*);
void (*pa_context_set_state_callback_dylibloader_wrapper_pulse)( pa_context*, pa_context_notify_cb_t, void*);
pa_context_state_t (*pa_context_get_state_dylibloader_wrapper_pulse)(const pa_context*);
int (*pa_context_connect_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_flags_t,const pa_spawn_api*);
void (*pa_context_disconnect_dylibloader_wrapper_pulse)( pa_context*);
pa_operation* (*pa_context_get_sink_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_sink_info_cb_t, void*);
pa_operation* (*pa_context_get_source_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_source_info_cb_t, void*);
pa_operation* (*pa_context_load_module_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_context_index_cb_t, void*);
pa_operation* (*pa_context_unload_module_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
const char* (*pa_strerror_dylibloader_wrapper_pulse)( int);
pa_mainloop* (*pa_mainloop_new_dylibloader_wrapper_pulse)( void);
void (*pa_mainloop_free_dylibloader_wrapper_pulse)( pa_mainloop*);
int (*pa_mainloop_iterate_dylibloader_wrapper_pulse)( pa_mainloop*, int, int*);
pa_mainloop_api* (*pa_mainloop_get_api_dylibloader_wrapper_pulse)( pa_mainloop*);
pa_simple* (*pa_simple_new_dylibloader_wrapper_pulse)(const char*,const char*, pa_stream_direction_t,const char*,const char*,const pa_sample_spec*,const pa_channel_map*,const pa_buffer_attr*, int*);
void (*pa_simple_free_dylibloader_wrapper_pulse)( pa_simple*);
int (*pa_simple_write_dylibloader_wrapper_pulse)( pa_simple*,const void*, size_t, int*);
int (*pa_simple_drain_dylibloader_wrapper_pulse)( pa_simple*, int*);
int (*pa_simple_read_dylibloader_wrapper_pulse)( pa_simple*, void*, size_t, int*);
pa_usec_t (*pa_simple_get_latency_dylibloader_wrapper_pulse)( pa_simple*, int*);
int (*pa_simple_flush_dylibloader_wrapper_pulse)( pa_simple*, int*);

int initialize_pulse(int verbose) {
  void *handle;
  char *error;
  handle = dlopen("libpulse-simple.so.0", RTLD_LAZY);
  if (!handle) {
    if (verbose) {
      fprintf(stderr, "%s\n", dlerror());
    }
    return(1);
  }
  dlerror();
// pa_operation_unref
  *(void **) (&pa_operation_unref_dylibloader_wrapper_pulse) = dlsym(handle, "pa_operation_unref");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_operation_get_state
  *(void **) (&pa_operation_get_state_dylibloader_wrapper_pulse) = dlsym(handle, "pa_operation_get_state");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_new
  *(void **) (&pa_context_new_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_new");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_unref
  *(void **) (&pa_context_unref_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_unref");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_set_state_callback
  *(void **) (&pa_context_set_state_callback_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_set_state_callback");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_get_state
  *(void **) (&pa_context_get_state_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_get_state");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_connect
  *(void **) (&pa_context_connect_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_connect");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_disconnect
  *(void **) (&pa_context_disconnect_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_disconnect");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_get_sink_info_list
  *(void **) (&pa_context_get_sink_info_list_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_get_sink_info_list");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_get_source_info_list
  *(void **) (&pa_context_get_source_info_list_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_get_source_info_list");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_load_module
  *(void **) (&pa_context_load_module_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_load_module");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_context_unload_module
  *(void **) (&pa_context_unload_module_dylibloader_wrapper_pulse) = dlsym(handle, "pa_context_unload_module");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_strerror
  *(void **) (&pa_strerror_dylibloader_wrapper_pulse) = dlsym(handle, "pa_strerror");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_mainloop_new
  *(void **) (&pa_mainloop_new_dylibloader_wrapper_pulse) = dlsym(handle, "pa_mainloop_new");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_mainloop_free
  *(void **) (&pa_mainloop_free_dylibloader_wrapper_pulse) = dlsym(handle, "pa_mainloop_free");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_mainloop_iterate
  *(void **) (&pa_mainloop_iterate_dylibloader_wrapper_pulse) = dlsym(handle, "pa_mainloop_iterate");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_mainloop_get_api
  *(void **) (&pa_mainloop_get_api_dylibloader_wrapper_pulse) = dlsym(handle, "pa_mainloop_get_api");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_new
  *(void **) (&pa_simple_new_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_new");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_free
  *(void **) (&pa_simple_free_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_free");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_write
  *(void **) (&pa_simple_write_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_write");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_drain
  *(void **) (&pa_simple_drain_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_drain");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_read
  *(void **) (&pa_simple_read_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_read");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_get_latency
  *(void **) (&pa_simple_get_latency_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_get_latency");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
// pa_simple_flush
  *(void **) (&pa_simple_flush_dylibloader_wrapper_pulse) = dlsym(handle, "pa_simple_flush");
  if (verbose) {
    error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "%s\n", error);
    }
  }
return 0;
}
