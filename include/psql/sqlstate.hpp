#pragma once

#include <boost/system/error_code.hpp>

namespace psql
{
enum class sqlstate
{
  successful_completion                                = 0,
  warning                                              = 46656,
  warning_dynamic_result_sets_returned                 = 46668,
  warning_implicit_zero_bit_padding                    = 46664,
  warning_null_value_eliminated_in_set_function        = 46659,
  warning_privilege_not_granted                        = 46663,
  warning_privilege_not_revoked                        = 46662,
  warning_string_data_right_truncation                 = 46660,
  warning_deprecated_feature                           = 79057,
  no_data                                              = 93312,
  no_additional_dynamic_result_sets_returned           = 93313,
  sql_statement_not_yet_complete                       = 139968,
  connection_exception                                 = 373248,
  connection_does_not_exist                            = 373251,
  connection_failure                                   = 373254,
  sqlclient_unable_to_establish_sqlconnection          = 373249,
  sqlserver_rejected_establishment_of_sqlconnection    = 373252,
  transaction_resolution_unknown                       = 373255,
  protocol_violation                                   = 405649,
  triggered_action_exception                           = 419904,
  feature_not_supported                                = 466560,
  invalid_transaction_initiation                       = 513216,
  locator_exception                                    = 699840,
  l_e_invalid_specification                            = 699841,
  invalid_grantor                                      = 979776,
  invalid_grant_operation                              = 1012177,
  invalid_role_specification                           = 1166400,
  diagnostics_exception                                = 1632960,
  stacked_diagnostics_accessed_without_active_handler  = 1632962,
  case_not_found                                       = 3359232,
  cardinality_violation                                = 3405888,
  data_exception                                       = 3452544,
  array_element_error                                  = 3452630,
  character_not_in_repertoire                          = 3452617,
  datetime_field_overflow                              = 3452552,
  division_by_zero                                     = 3452582,
  error_in_assignment                                  = 3452549,
  escape_character_conflict                            = 3452555,
  indicator_overflow                                   = 3452618,
  interval_field_overflow                              = 3452585,
  invalid_argument_for_log                             = 3452594,
  invalid_argument_for_ntile                           = 3452584,
  invalid_argument_for_nth_value                       = 3452586,
  invalid_argument_for_power_function                  = 3452595,
  invalid_argument_for_width_bucket_function           = 3452596,
  invalid_character_value_for_cast                     = 3452588,
  invalid_datetime_format                              = 3452551,
  invalid_escape_character                             = 3452589,
  invalid_escape_octet                                 = 3452557,
  invalid_escape_sequence                              = 3452621,
  nonstandard_use_of_escape_character                  = 3484950,
  invalid_indicator_parameter_value                    = 3452580,
  invalid_parameter_value                              = 3452619,
  invalid_preceding_or_following_size                  = 3452583,
  invalid_regular_expression                           = 3452591,
  invalid_row_count_in_limit_clause                    = 3452612,
  invalid_row_count_in_result_offset_clause            = 3452613,
  invalid_tablesample_argument                         = 3452633,
  invalid_tablesample_repeat                           = 3452632,
  invalid_time_zone_displacement_value                 = 3452553,
  invalid_use_of_escape_character                      = 3452556,
  most_specific_type_mismatch                          = 3452560,
  null_value_not_allowed                               = 3452548,
  null_value_no_indicator_parameter                    = 3452546,
  numeric_value_out_of_range                           = 3452547,
  sequence_generator_limit_exceeded                    = 3452561,
  string_data_length_mismatch                          = 3452622,
  string_data_right_truncation                         = 3452545,
  substring_error                                      = 3452581,
  trim_error                                           = 3452623,
  unterminated_c_string                                = 3452620,
  zero_length_character_string                         = 3452559,
  floating_point_exception                             = 3484945,
  invalid_text_representation                          = 3484946,
  invalid_binary_representation                        = 3484947,
  bad_copy_file_format                                 = 3484948,
  untranslatable_character                             = 3484949,
  not_an_xml_document                                  = 3452565,
  invalid_xml_document                                 = 3452566,
  invalid_xml_content                                  = 3452567,
  invalid_xml_comment                                  = 3452572,
  invalid_xml_processing_instruction                   = 3452573,
  duplicate_json_object_key_value                      = 3452652,
  invalid_argument_for_sql_json_datetime_function      = 3452653,
  invalid_json_text                                    = 3452654,
  invalid_sql_json_subscript                           = 3452655,
  more_than_one_sql_json_item                          = 3452656,
  no_sql_json_item                                     = 3452657,
  non_numeric_sql_json_item                            = 3452658,
  non_unique_keys_in_a_json_object                     = 3452659,
  singleton_sql_json_item_required                     = 3452660,
  sql_json_array_not_found                             = 3452661,
  sql_json_member_not_found                            = 3452662,
  sql_json_number_not_found                            = 3452663,
  sql_json_object_not_found                            = 3452664,
  too_many_json_array_elements                         = 3452665,
  too_many_json_object_members                         = 3452666,
  sql_json_scalar_required                             = 3452667,
  sql_json_item_cannot_be_cast_to_target_type          = 3452668,
  integrity_constraint_violation                       = 3499200,
  restrict_violation                                   = 3499201,
  not_null_violation                                   = 3505682,
  foreign_key_violation                                = 3505683,
  unique_violation                                     = 3505685,
  check_violation                                      = 3505720,
  exclusion_violation                                  = 3531601,
  invalid_cursor_state                                 = 3545856,
  invalid_transaction_state                            = 3592512,
  active_sql_transaction                               = 3592513,
  branch_transaction_already_active                    = 3592514,
  held_cursor_requires_same_isolation_level            = 3592520,
  inappropriate_access_mode_for_branch_transaction     = 3592515,
  inappropriate_isolation_level_for_branch_transaction = 3592516,
  no_active_sql_transaction_for_branch_transaction     = 3592517,
  read_only_sql_transaction                            = 3592518,
  schema_and_data_statement_mixing_not_supported       = 3592519,
  no_active_sql_transaction                            = 3624913,
  in_failed_sql_transaction                            = 3624914,
  idle_in_transaction_session_timeout                  = 3624915,
  invalid_sql_statement_name                           = 3639168,
  triggered_data_change_violation                      = 3685824,
  invalid_authorization_specification                  = 3732480,
  invalid_password                                     = 3764881,
  dependent_privilege_descriptors_still_exist          = 3872448,
  dependent_objects_still_exist                        = 3904849,
  invalid_transaction_termination                      = 3965760,
  sql_routine_exception                                = 4059072,
  s_r_e_function_executed_no_return_statement          = 4059077,
  s_r_e_modifying_sql_data_not_permitted               = 4059074,
  s_r_e_prohibited_sql_statement_attempted             = 4059075,
  s_r_e_reading_sql_data_not_permitted                 = 4059076,
  invalid_cursor_name                                  = 5225472,
  external_routine_exception                           = 5412096,
  e_r_e_containing_sql_not_permitted                   = 5412097,
  e_r_e_modifying_sql_data_not_permitted               = 5412098,
  e_r_e_prohibited_sql_statement_attempted             = 5412099,
  e_r_e_reading_sql_data_not_permitted                 = 5412100,
  external_routine_invocation_exception                = 5458752,
  e_r_i_e_invalid_sqlstate_returned                    = 5458753,
  e_r_i_e_null_value_not_allowed                       = 5458756,
  e_r_i_e_trigger_protocol_violated                    = 5491153,
  e_r_i_e_srf_protocol_violated                        = 5491154,
  e_r_i_e_event_trigger_protocol_violated              = 5491155,
  savepoint_exception                                  = 5552064,
  s_e_invalid_specification                            = 5552065,
  invalid_catalog_name                                 = 5645376,
  invalid_schema_name                                  = 5738688,
  transaction_rollback                                 = 6718464,
  t_r_integrity_constraint_violation                   = 6718466,
  t_r_serialization_failure                            = 6718465,
  t_r_statement_completion_unknown                     = 6718467,
  t_r_deadlock_detected                                = 6750865,
  syntax_error_or_access_rule_violation                = 6811776,
  syntax_error                                         = 6819553,
  insufficient_privilege                               = 6818257,
  cannot_coerce                                        = 6822294,
  grouping_error                                       = 6822147,
  windowing_error                                      = 6844248,
  invalid_recursion                                    = 6844221,
  invalid_foreign_key                                  = 6822252,
  invalid_name                                         = 6819554,
  name_too_long                                        = 6819626,
  reserved_name                                        = 6823557,
  datatype_mismatch                                    = 6822148,
  indeterminate_datatype                               = 6844220,
  collation_mismatch                                   = 6844249,
  indeterminate_collation                              = 6844250,
  wrong_object_type                                    = 6822153,
  generated_always                                     = 6822585,
  undefined_column                                     = 6820851,
  undefined_function                                   = 6822435,
  undefined_table                                      = 6844177,
  undefined_parameter                                  = 6844178,
  undefined_object                                     = 6820852,
  duplicate_column                                     = 6820849,
  duplicate_cursor                                     = 6844179,
  duplicate_database                                   = 6844180,
  duplicate_function                                   = 6820923,
  duplicate_pstatement                                 = 6844181,
  duplicate_schema                                     = 6844182,
  duplicate_table                                      = 6844183,
  duplicate_alias                                      = 6820886,
  duplicate_object                                     = 6820884,
  ambiguous_column                                     = 6820850,
  ambiguous_function                                   = 6820925,
  ambiguous_parameter                                  = 6844184,
  ambiguous_alias                                      = 6844185,
  invalid_column_reference                             = 6844212,
  invalid_column_definition                            = 6819589,
  invalid_cursor_definition                            = 6844213,
  invalid_database_definition                          = 6844214,
  invalid_function_definition                          = 6844215,
  invalid_pstatement_definition                        = 6844216,
  invalid_schema_definition                            = 6844217,
  invalid_table_definition                             = 6844218,
  invalid_object_definition                            = 6844219,
  with_check_option_violation                          = 6905088,
  insufficient_resources                               = 8538048,
  disk_full                                            = 8539344,
  out_of_memory                                        = 8540640,
  too_many_connections                                 = 8541936,
  configuration_limit_exceeded                         = 8543232,
  program_limit_exceeded                               = 8584704,
  statement_too_complex                                = 8584705,
  too_many_columns                                     = 8584741,
  too_many_arguments                                   = 8584779,
  object_not_in_prerequisite_state                     = 8631360,
  object_in_use                                        = 8631366,
  cant_change_runtime_param                            = 8663762,
  lock_not_available                                   = 8663763,
  unsafe_new_enum_value_usage                          = 8663764,
  operator_intervention                                = 8724672,
  query_canceled                                       = 8724712,
  admin_shutdown                                       = 8757073,
  crash_shutdown                                       = 8757074,
  cannot_connect_now                                   = 8757075,
  database_dropped                                     = 8757076,
  idle_session_timeout                                 = 8757077,
  system_error                                         = 8771328,
  io_error                                             = 8771436,
  undefined_file                                       = 8803729,
  duplicate_file                                       = 8803730,
  snapshot_too_old                                     = 11850624,
  config_file_error                                    = 25194240,
  lock_file_exists                                     = 25194241,
  fdw_error                                            = 29999808,
  fdw_column_name_not_found                            = 29999813,
  fdw_dynamic_parameter_value_needed                   = 29999810,
  fdw_function_sequence_error                          = 29999844,
  fdw_inconsistent_descriptor_information              = 29999881,
  fdw_invalid_attribute_value                          = 29999884,
  fdw_invalid_column_name                              = 29999815,
  fdw_invalid_column_number                            = 29999816,
  fdw_invalid_data_type                                = 29999812,
  fdw_invalid_data_type_descriptors                    = 29999814,
  fdw_invalid_descriptor_field_identifier              = 30000133,
  fdw_invalid_handle                                   = 29999819,
  fdw_invalid_option_index                             = 29999820,
  fdw_invalid_option_name                              = 29999821,
  fdw_invalid_string_length_or_buffer_length           = 30000132,
  fdw_invalid_string_format                            = 29999818,
  fdw_invalid_use_of_null_pointer                      = 29999817,
  fdw_too_many_handles                                 = 29999848,
  fdw_out_of_memory                                    = 29999809,
  fdw_no_schemas                                       = 29999833,
  fdw_option_name_not_found                            = 29999827,
  fdw_reply_handle                                     = 29999828,
  fdw_schema_not_found                                 = 29999834,
  fdw_table_not_found                                  = 29999835,
  fdw_unable_to_create_execution                       = 29999829,
  fdw_unable_to_create_reply                           = 29999830,
  fdw_unable_to_establish_connection                   = 29999831,
  plpgsql_error                                        = 41990400,
  raise_exception                                      = 41990401,
  no_data_found                                        = 41990402,
  too_many_rows                                        = 41990403,
  assert_failure                                       = 41990404,
  internal_error                                       = 56966976,
  data_corrupted                                       = 56966977,
  index_corrupted                                      = 56966978,
};

inline const boost::system::error_category& sqlstate_category()
{
  struct category : boost::system::error_category
  {
    virtual ~category() = default;

    const char* name() const noexcept override
    {
      return "sqlstate";
    }

    std::string message(int ev) const override
    {
      switch (static_cast<sqlstate>(ev))
      {
        case sqlstate::successful_completion:
          return "successful_completion";
        case sqlstate::warning:
          return "warning";
        case sqlstate::warning_dynamic_result_sets_returned:
          return "warning_dynamic_result_sets_returned";
        case sqlstate::warning_implicit_zero_bit_padding:
          return "warning_implicit_zero_bit_padding";
        case sqlstate::warning_null_value_eliminated_in_set_function:
          return "warning_null_value_eliminated_in_set_function";
        case sqlstate::warning_privilege_not_granted:
          return "warning_privilege_not_granted";
        case sqlstate::warning_privilege_not_revoked:
          return "warning_privilege_not_revoked";
        case sqlstate::warning_string_data_right_truncation:
          return "warning_string_data_right_truncation";
        case sqlstate::warning_deprecated_feature:
          return "warning_deprecated_feature";
        case sqlstate::no_data:
          return "no_data";
        case sqlstate::no_additional_dynamic_result_sets_returned:
          return "no_additional_dynamic_result_sets_returned";
        case sqlstate::sql_statement_not_yet_complete:
          return "sql_statement_not_yet_complete";
        case sqlstate::connection_exception:
          return "connection_exception";
        case sqlstate::connection_does_not_exist:
          return "connection_does_not_exist";
        case sqlstate::connection_failure:
          return "connection_failure";
        case sqlstate::sqlclient_unable_to_establish_sqlconnection:
          return "sqlclient_unable_to_establish_sqlconnection";
        case sqlstate::sqlserver_rejected_establishment_of_sqlconnection:
          return "sqlserver_rejected_establishment_of_sqlconnection";
        case sqlstate::transaction_resolution_unknown:
          return "transaction_resolution_unknown";
        case sqlstate::protocol_violation:
          return "protocol_violation";
        case sqlstate::triggered_action_exception:
          return "triggered_action_exception";
        case sqlstate::feature_not_supported:
          return "feature_not_supported";
        case sqlstate::invalid_transaction_initiation:
          return "invalid_transaction_initiation";
        case sqlstate::locator_exception:
          return "locator_exception";
        case sqlstate::l_e_invalid_specification:
          return "l_e_invalid_specification";
        case sqlstate::invalid_grantor:
          return "invalid_grantor";
        case sqlstate::invalid_grant_operation:
          return "invalid_grant_operation";
        case sqlstate::invalid_role_specification:
          return "invalid_role_specification";
        case sqlstate::diagnostics_exception:
          return "diagnostics_exception";
        case sqlstate::stacked_diagnostics_accessed_without_active_handler:
          return "stacked_diagnostics_accessed_without_active_handler";
        case sqlstate::case_not_found:
          return "case_not_found";
        case sqlstate::cardinality_violation:
          return "cardinality_violation";
        case sqlstate::data_exception:
          return "data_exception";
        case sqlstate::array_element_error:
          return "array_element_error";
        case sqlstate::character_not_in_repertoire:
          return "character_not_in_repertoire";
        case sqlstate::datetime_field_overflow:
          return "datetime_field_overflow";
        case sqlstate::division_by_zero:
          return "division_by_zero";
        case sqlstate::error_in_assignment:
          return "error_in_assignment";
        case sqlstate::escape_character_conflict:
          return "escape_character_conflict";
        case sqlstate::indicator_overflow:
          return "indicator_overflow";
        case sqlstate::interval_field_overflow:
          return "interval_field_overflow";
        case sqlstate::invalid_argument_for_log:
          return "invalid_argument_for_log";
        case sqlstate::invalid_argument_for_ntile:
          return "invalid_argument_for_ntile";
        case sqlstate::invalid_argument_for_nth_value:
          return "invalid_argument_for_nth_value";
        case sqlstate::invalid_argument_for_power_function:
          return "invalid_argument_for_power_function";
        case sqlstate::invalid_argument_for_width_bucket_function:
          return "invalid_argument_for_width_bucket_function";
        case sqlstate::invalid_character_value_for_cast:
          return "invalid_character_value_for_cast";
        case sqlstate::invalid_datetime_format:
          return "invalid_datetime_format";
        case sqlstate::invalid_escape_character:
          return "invalid_escape_character";
        case sqlstate::invalid_escape_octet:
          return "invalid_escape_octet";
        case sqlstate::invalid_escape_sequence:
          return "invalid_escape_sequence";
        case sqlstate::nonstandard_use_of_escape_character:
          return "nonstandard_use_of_escape_character";
        case sqlstate::invalid_indicator_parameter_value:
          return "invalid_indicator_parameter_value";
        case sqlstate::invalid_parameter_value:
          return "invalid_parameter_value";
        case sqlstate::invalid_preceding_or_following_size:
          return "invalid_preceding_or_following_size";
        case sqlstate::invalid_regular_expression:
          return "invalid_regular_expression";
        case sqlstate::invalid_row_count_in_limit_clause:
          return "invalid_row_count_in_limit_clause";
        case sqlstate::invalid_row_count_in_result_offset_clause:
          return "invalid_row_count_in_result_offset_clause";
        case sqlstate::invalid_tablesample_argument:
          return "invalid_tablesample_argument";
        case sqlstate::invalid_tablesample_repeat:
          return "invalid_tablesample_repeat";
        case sqlstate::invalid_time_zone_displacement_value:
          return "invalid_time_zone_displacement_value";
        case sqlstate::invalid_use_of_escape_character:
          return "invalid_use_of_escape_character";
        case sqlstate::most_specific_type_mismatch:
          return "most_specific_type_mismatch";
        case sqlstate::null_value_not_allowed:
          return "null_value_not_allowed";
        case sqlstate::null_value_no_indicator_parameter:
          return "null_value_no_indicator_parameter";
        case sqlstate::numeric_value_out_of_range:
          return "numeric_value_out_of_range";
        case sqlstate::sequence_generator_limit_exceeded:
          return "sequence_generator_limit_exceeded";
        case sqlstate::string_data_length_mismatch:
          return "string_data_length_mismatch";
        case sqlstate::string_data_right_truncation:
          return "string_data_right_truncation";
        case sqlstate::substring_error:
          return "substring_error";
        case sqlstate::trim_error:
          return "trim_error";
        case sqlstate::unterminated_c_string:
          return "unterminated_c_string";
        case sqlstate::zero_length_character_string:
          return "zero_length_character_string";
        case sqlstate::floating_point_exception:
          return "floating_point_exception";
        case sqlstate::invalid_text_representation:
          return "invalid_text_representation";
        case sqlstate::invalid_binary_representation:
          return "invalid_binary_representation";
        case sqlstate::bad_copy_file_format:
          return "bad_copy_file_format";
        case sqlstate::untranslatable_character:
          return "untranslatable_character";
        case sqlstate::not_an_xml_document:
          return "not_an_xml_document";
        case sqlstate::invalid_xml_document:
          return "invalid_xml_document";
        case sqlstate::invalid_xml_content:
          return "invalid_xml_content";
        case sqlstate::invalid_xml_comment:
          return "invalid_xml_comment";
        case sqlstate::invalid_xml_processing_instruction:
          return "invalid_xml_processing_instruction";
        case sqlstate::duplicate_json_object_key_value:
          return "duplicate_json_object_key_value";
        case sqlstate::invalid_argument_for_sql_json_datetime_function:
          return "invalid_argument_for_sql_json_datetime_function";
        case sqlstate::invalid_json_text:
          return "invalid_json_text";
        case sqlstate::invalid_sql_json_subscript:
          return "invalid_sql_json_subscript";
        case sqlstate::more_than_one_sql_json_item:
          return "more_than_one_sql_json_item";
        case sqlstate::no_sql_json_item:
          return "no_sql_json_item";
        case sqlstate::non_numeric_sql_json_item:
          return "non_numeric_sql_json_item";
        case sqlstate::non_unique_keys_in_a_json_object:
          return "non_unique_keys_in_a_json_object";
        case sqlstate::singleton_sql_json_item_required:
          return "singleton_sql_json_item_required";
        case sqlstate::sql_json_array_not_found:
          return "sql_json_array_not_found";
        case sqlstate::sql_json_member_not_found:
          return "sql_json_member_not_found";
        case sqlstate::sql_json_number_not_found:
          return "sql_json_number_not_found";
        case sqlstate::sql_json_object_not_found:
          return "sql_json_object_not_found";
        case sqlstate::too_many_json_array_elements:
          return "too_many_json_array_elements";
        case sqlstate::too_many_json_object_members:
          return "too_many_json_object_members";
        case sqlstate::sql_json_scalar_required:
          return "sql_json_scalar_required";
        case sqlstate::sql_json_item_cannot_be_cast_to_target_type:
          return "sql_json_item_cannot_be_cast_to_target_type";
        case sqlstate::integrity_constraint_violation:
          return "integrity_constraint_violation";
        case sqlstate::restrict_violation:
          return "restrict_violation";
        case sqlstate::not_null_violation:
          return "not_null_violation";
        case sqlstate::foreign_key_violation:
          return "foreign_key_violation";
        case sqlstate::unique_violation:
          return "unique_violation";
        case sqlstate::check_violation:
          return "check_violation";
        case sqlstate::exclusion_violation:
          return "exclusion_violation";
        case sqlstate::invalid_cursor_state:
          return "invalid_cursor_state";
        case sqlstate::invalid_transaction_state:
          return "invalid_transaction_state";
        case sqlstate::active_sql_transaction:
          return "active_sql_transaction";
        case sqlstate::branch_transaction_already_active:
          return "branch_transaction_already_active";
        case sqlstate::held_cursor_requires_same_isolation_level:
          return "held_cursor_requires_same_isolation_level";
        case sqlstate::inappropriate_access_mode_for_branch_transaction:
          return "inappropriate_access_mode_for_branch_transaction";
        case sqlstate::inappropriate_isolation_level_for_branch_transaction:
          return "inappropriate_isolation_level_for_branch_transaction";
        case sqlstate::no_active_sql_transaction_for_branch_transaction:
          return "no_active_sql_transaction_for_branch_transaction";
        case sqlstate::read_only_sql_transaction:
          return "read_only_sql_transaction";
        case sqlstate::schema_and_data_statement_mixing_not_supported:
          return "schema_and_data_statement_mixing_not_supported";
        case sqlstate::no_active_sql_transaction:
          return "no_active_sql_transaction";
        case sqlstate::in_failed_sql_transaction:
          return "in_failed_sql_transaction";
        case sqlstate::idle_in_transaction_session_timeout:
          return "idle_in_transaction_session_timeout";
        case sqlstate::invalid_sql_statement_name:
          return "invalid_sql_statement_name";
        case sqlstate::triggered_data_change_violation:
          return "triggered_data_change_violation";
        case sqlstate::invalid_authorization_specification:
          return "invalid_authorization_specification";
        case sqlstate::invalid_password:
          return "invalid_password";
        case sqlstate::dependent_privilege_descriptors_still_exist:
          return "dependent_privilege_descriptors_still_exist";
        case sqlstate::dependent_objects_still_exist:
          return "dependent_objects_still_exist";
        case sqlstate::invalid_transaction_termination:
          return "invalid_transaction_termination";
        case sqlstate::sql_routine_exception:
          return "sql_routine_exception";
        case sqlstate::s_r_e_function_executed_no_return_statement:
          return "s_r_e_function_executed_no_return_statement";
        case sqlstate::s_r_e_modifying_sql_data_not_permitted:
          return "s_r_e_modifying_sql_data_not_permitted";
        case sqlstate::s_r_e_prohibited_sql_statement_attempted:
          return "s_r_e_prohibited_sql_statement_attempted";
        case sqlstate::s_r_e_reading_sql_data_not_permitted:
          return "s_r_e_reading_sql_data_not_permitted";
        case sqlstate::invalid_cursor_name:
          return "invalid_cursor_name";
        case sqlstate::external_routine_exception:
          return "external_routine_exception";
        case sqlstate::e_r_e_containing_sql_not_permitted:
          return "e_r_e_containing_sql_not_permitted";
        case sqlstate::e_r_e_modifying_sql_data_not_permitted:
          return "e_r_e_modifying_sql_data_not_permitted";
        case sqlstate::e_r_e_prohibited_sql_statement_attempted:
          return "e_r_e_prohibited_sql_statement_attempted";
        case sqlstate::e_r_e_reading_sql_data_not_permitted:
          return "e_r_e_reading_sql_data_not_permitted";
        case sqlstate::external_routine_invocation_exception:
          return "external_routine_invocation_exception";
        case sqlstate::e_r_i_e_invalid_sqlstate_returned:
          return "e_r_i_e_invalid_sqlstate_returned";
        case sqlstate::e_r_i_e_null_value_not_allowed:
          return "e_r_i_e_null_value_not_allowed";
        case sqlstate::e_r_i_e_trigger_protocol_violated:
          return "e_r_i_e_trigger_protocol_violated";
        case sqlstate::e_r_i_e_srf_protocol_violated:
          return "e_r_i_e_srf_protocol_violated";
        case sqlstate::e_r_i_e_event_trigger_protocol_violated:
          return "e_r_i_e_event_trigger_protocol_violated";
        case sqlstate::savepoint_exception:
          return "savepoint_exception";
        case sqlstate::s_e_invalid_specification:
          return "s_e_invalid_specification";
        case sqlstate::invalid_catalog_name:
          return "invalid_catalog_name";
        case sqlstate::invalid_schema_name:
          return "invalid_schema_name";
        case sqlstate::transaction_rollback:
          return "transaction_rollback";
        case sqlstate::t_r_integrity_constraint_violation:
          return "t_r_integrity_constraint_violation";
        case sqlstate::t_r_serialization_failure:
          return "t_r_serialization_failure";
        case sqlstate::t_r_statement_completion_unknown:
          return "t_r_statement_completion_unknown";
        case sqlstate::t_r_deadlock_detected:
          return "t_r_deadlock_detected";
        case sqlstate::syntax_error_or_access_rule_violation:
          return "syntax_error_or_access_rule_violation";
        case sqlstate::syntax_error:
          return "syntax_error";
        case sqlstate::insufficient_privilege:
          return "insufficient_privilege";
        case sqlstate::cannot_coerce:
          return "cannot_coerce";
        case sqlstate::grouping_error:
          return "grouping_error";
        case sqlstate::windowing_error:
          return "windowing_error";
        case sqlstate::invalid_recursion:
          return "invalid_recursion";
        case sqlstate::invalid_foreign_key:
          return "invalid_foreign_key";
        case sqlstate::invalid_name:
          return "invalid_name";
        case sqlstate::name_too_long:
          return "name_too_long";
        case sqlstate::reserved_name:
          return "reserved_name";
        case sqlstate::datatype_mismatch:
          return "datatype_mismatch";
        case sqlstate::indeterminate_datatype:
          return "indeterminate_datatype";
        case sqlstate::collation_mismatch:
          return "collation_mismatch";
        case sqlstate::indeterminate_collation:
          return "indeterminate_collation";
        case sqlstate::wrong_object_type:
          return "wrong_object_type";
        case sqlstate::generated_always:
          return "generated_always";
        case sqlstate::undefined_column:
          return "undefined_column";
        case sqlstate::undefined_function:
          return "undefined_function";
        case sqlstate::undefined_table:
          return "undefined_table";
        case sqlstate::undefined_parameter:
          return "undefined_parameter";
        case sqlstate::undefined_object:
          return "undefined_object";
        case sqlstate::duplicate_column:
          return "duplicate_column";
        case sqlstate::duplicate_cursor:
          return "duplicate_cursor";
        case sqlstate::duplicate_database:
          return "duplicate_database";
        case sqlstate::duplicate_function:
          return "duplicate_function";
        case sqlstate::duplicate_pstatement:
          return "duplicate_pstatement";
        case sqlstate::duplicate_schema:
          return "duplicate_schema";
        case sqlstate::duplicate_table:
          return "duplicate_table";
        case sqlstate::duplicate_alias:
          return "duplicate_alias";
        case sqlstate::duplicate_object:
          return "duplicate_object";
        case sqlstate::ambiguous_column:
          return "ambiguous_column";
        case sqlstate::ambiguous_function:
          return "ambiguous_function";
        case sqlstate::ambiguous_parameter:
          return "ambiguous_parameter";
        case sqlstate::ambiguous_alias:
          return "ambiguous_alias";
        case sqlstate::invalid_column_reference:
          return "invalid_column_reference";
        case sqlstate::invalid_column_definition:
          return "invalid_column_definition";
        case sqlstate::invalid_cursor_definition:
          return "invalid_cursor_definition";
        case sqlstate::invalid_database_definition:
          return "invalid_database_definition";
        case sqlstate::invalid_function_definition:
          return "invalid_function_definition";
        case sqlstate::invalid_pstatement_definition:
          return "invalid_pstatement_definition";
        case sqlstate::invalid_schema_definition:
          return "invalid_schema_definition";
        case sqlstate::invalid_table_definition:
          return "invalid_table_definition";
        case sqlstate::invalid_object_definition:
          return "invalid_object_definition";
        case sqlstate::with_check_option_violation:
          return "with_check_option_violation";
        case sqlstate::insufficient_resources:
          return "insufficient_resources";
        case sqlstate::disk_full:
          return "disk_full";
        case sqlstate::out_of_memory:
          return "out_of_memory";
        case sqlstate::too_many_connections:
          return "too_many_connections";
        case sqlstate::configuration_limit_exceeded:
          return "configuration_limit_exceeded";
        case sqlstate::program_limit_exceeded:
          return "program_limit_exceeded";
        case sqlstate::statement_too_complex:
          return "statement_too_complex";
        case sqlstate::too_many_columns:
          return "too_many_columns";
        case sqlstate::too_many_arguments:
          return "too_many_arguments";
        case sqlstate::object_not_in_prerequisite_state:
          return "object_not_in_prerequisite_state";
        case sqlstate::object_in_use:
          return "object_in_use";
        case sqlstate::cant_change_runtime_param:
          return "cant_change_runtime_param";
        case sqlstate::lock_not_available:
          return "lock_not_available";
        case sqlstate::unsafe_new_enum_value_usage:
          return "unsafe_new_enum_value_usage";
        case sqlstate::operator_intervention:
          return "operator_intervention";
        case sqlstate::query_canceled:
          return "query_canceled";
        case sqlstate::admin_shutdown:
          return "admin_shutdown";
        case sqlstate::crash_shutdown:
          return "crash_shutdown";
        case sqlstate::cannot_connect_now:
          return "cannot_connect_now";
        case sqlstate::database_dropped:
          return "database_dropped";
        case sqlstate::idle_session_timeout:
          return "idle_session_timeout";
        case sqlstate::system_error:
          return "system_error";
        case sqlstate::io_error:
          return "io_error";
        case sqlstate::undefined_file:
          return "undefined_file";
        case sqlstate::duplicate_file:
          return "duplicate_file";
        case sqlstate::snapshot_too_old:
          return "snapshot_too_old";
        case sqlstate::config_file_error:
          return "config_file_error";
        case sqlstate::lock_file_exists:
          return "lock_file_exists";
        case sqlstate::fdw_error:
          return "fdw_error";
        case sqlstate::fdw_column_name_not_found:
          return "fdw_column_name_not_found";
        case sqlstate::fdw_dynamic_parameter_value_needed:
          return "fdw_dynamic_parameter_value_needed";
        case sqlstate::fdw_function_sequence_error:
          return "fdw_function_sequence_error";
        case sqlstate::fdw_inconsistent_descriptor_information:
          return "fdw_inconsistent_descriptor_information";
        case sqlstate::fdw_invalid_attribute_value:
          return "fdw_invalid_attribute_value";
        case sqlstate::fdw_invalid_column_name:
          return "fdw_invalid_column_name";
        case sqlstate::fdw_invalid_column_number:
          return "fdw_invalid_column_number";
        case sqlstate::fdw_invalid_data_type:
          return "fdw_invalid_data_type";
        case sqlstate::fdw_invalid_data_type_descriptors:
          return "fdw_invalid_data_type_descriptors";
        case sqlstate::fdw_invalid_descriptor_field_identifier:
          return "fdw_invalid_descriptor_field_identifier";
        case sqlstate::fdw_invalid_handle:
          return "fdw_invalid_handle";
        case sqlstate::fdw_invalid_option_index:
          return "fdw_invalid_option_index";
        case sqlstate::fdw_invalid_option_name:
          return "fdw_invalid_option_name";
        case sqlstate::fdw_invalid_string_length_or_buffer_length:
          return "fdw_invalid_string_length_or_buffer_length";
        case sqlstate::fdw_invalid_string_format:
          return "fdw_invalid_string_format";
        case sqlstate::fdw_invalid_use_of_null_pointer:
          return "fdw_invalid_use_of_null_pointer";
        case sqlstate::fdw_too_many_handles:
          return "fdw_too_many_handles";
        case sqlstate::fdw_out_of_memory:
          return "fdw_out_of_memory";
        case sqlstate::fdw_no_schemas:
          return "fdw_no_schemas";
        case sqlstate::fdw_option_name_not_found:
          return "fdw_option_name_not_found";
        case sqlstate::fdw_reply_handle:
          return "fdw_reply_handle";
        case sqlstate::fdw_schema_not_found:
          return "fdw_schema_not_found";
        case sqlstate::fdw_table_not_found:
          return "fdw_table_not_found";
        case sqlstate::fdw_unable_to_create_execution:
          return "fdw_unable_to_create_execution";
        case sqlstate::fdw_unable_to_create_reply:
          return "fdw_unable_to_create_reply";
        case sqlstate::fdw_unable_to_establish_connection:
          return "fdw_unable_to_establish_connection";
        case sqlstate::plpgsql_error:
          return "plpgsql_error";
        case sqlstate::raise_exception:
          return "raise_exception";
        case sqlstate::no_data_found:
          return "no_data_found";
        case sqlstate::too_many_rows:
          return "too_many_rows";
        case sqlstate::assert_failure:
          return "assert_failure";
        case sqlstate::internal_error:
          return "internal_error";
        case sqlstate::data_corrupted:
          return "data_corrupted";
        case sqlstate::index_corrupted:
          return "index_corrupted";
        default:
          return "Unknown sqlstate";
      }
    }
  };

  static const auto category_ = category{};

  return category_;
};

inline boost::system::error_code make_error_code(sqlstate e)
{
  return { static_cast<int>(e), sqlstate_category() };
}
} // namespace psql

namespace boost::system
{
template<>
struct is_error_code_enum<psql::sqlstate>
{
  static const bool value = true;
};
} // namespace boost::system
