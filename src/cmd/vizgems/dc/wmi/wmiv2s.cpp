
#define _vsnprintf_s(a,b,c,d,e) vsnprintf(a,c,d,e)
#define _vsnprintf vsnprintf
#define sprintf_s snprintf

#include <wchar.h>
#include <comdef.h>
#include <comutil.h>
#include <strsafe.h>
#include <Wbemidl.h>
#include <ast/sfio.h>

static inline const char *b2s (BSTR b) {
  return _com_util::ConvertBSTRToString (b);
}
static inline BSTR s2b (const char* ccp) {
  return _com_util::ConvertStringToBSTR (ccp);
}

static char v2sbuf[1024];

const char *E2ccp (HRESULT hr) {
  HRESULT ehr = 0;
  _com_error ce(hr);

  switch (hr) {
  case WBEM_NO_ERROR:
    return "WBEM_NO_ERROR";
  case WBEM_S_FALSE:
    return "WBEM_S_FALSE";
  case WBEM_S_ALREADY_EXISTS:
    return "WBEM_S_ALREADY_EXISTS";
  case WBEM_S_RESET_TO_DEFAULT:
    return "WBEM_S_RESET_TO_DEFAULT";
  case WBEM_S_DIFFERENT:
    return "WBEM_S_DIFFERENT";
  case WBEM_S_TIMEDOUT:
    return "WBEM_S_TIMEDOUT";
  case WBEM_S_NO_MORE_DATA:
    return "WBEM_S_NO_MORE_DATA";
  case WBEM_S_OPERATION_CANCELLED:
    return "WBEM_S_OPERATION_CANCELLED";
  case WBEM_S_PENDING:
    return "WBEM_S_PENDING";
  case WBEM_S_DUPLICATE_OBJECTS:
    return "WBEM_S_DUPLICATE_OBJECTS";
  case WBEM_S_ACCESS_DENIED:
    return "WBEM_S_ACCESS_DENIED";
  case WBEM_S_PARTIAL_RESULTS:
    return "WBEM_S_PARTIAL_RESULTS";
  case WBEM_S_SOURCE_NOT_AVAILABLE:
    return "WBEM_S_SOURCE_NOT_AVAILABLE";
  case WBEM_E_FAILED:
    return "WBEM_E_FAILED";
  case WBEM_E_NOT_FOUND:
    return "WBEM_E_NOT_FOUND";
  case WBEM_E_ACCESS_DENIED:
    return "WBEM_E_ACCESS_DENIED";
  case WBEM_E_PROVIDER_FAILURE:
    return "WBEM_E_PROVIDER_FAILURE";
  case WBEM_E_TYPE_MISMATCH:
    return "WBEM_E_TYPE_MISMATCH";
  case WBEM_E_OUT_OF_MEMORY:
    return "WBEM_E_OUT_OF_MEMORY";
  case WBEM_E_INVALID_CONTEXT:
    return "WBEM_E_INVALID_CONTEXT";
  case WBEM_E_INVALID_PARAMETER:
    return "WBEM_E_INVALID_PARAMETER";
  case WBEM_E_NOT_AVAILABLE:
    return "WBEM_E_NOT_AVAILABLE";
  case WBEM_E_CRITICAL_ERROR:
    return "WBEM_E_CRITICAL_ERROR";
  case WBEM_E_INVALID_STREAM:
    return "WBEM_E_INVALID_STREAM";
  case WBEM_E_NOT_SUPPORTED:
    return "WBEM_E_NOT_SUPPORTED";
  case WBEM_E_INVALID_SUPERCLASS:
    return "WBEM_E_INVALID_SUPERCLASS";
  case WBEM_E_INVALID_NAMESPACE:
    return "WBEM_E_INVALID_NAMESPACE";
  case WBEM_E_INVALID_OBJECT:
    return "WBEM_E_INVALID_OBJECT";
  case WBEM_E_INVALID_CLASS:
    return "WBEM_E_INVALID_CLASS";
  case WBEM_E_PROVIDER_NOT_FOUND:
    return "WBEM_E_PROVIDER_NOT_FOUND";
  case WBEM_E_INVALID_PROVIDER_REGISTRATION:
    return "WBEM_E_INVALID_PROVIDER_REGISTRATION";
  case WBEM_E_PROVIDER_LOAD_FAILURE:
    return "WBEM_E_PROVIDER_LOAD_FAILURE";
  case WBEM_E_INITIALIZATION_FAILURE:
    return "WBEM_E_INITIALIZATION_FAILURE";
  case WBEM_E_TRANSPORT_FAILURE:
    return "WBEM_E_TRANSPORT_FAILURE";
  case WBEM_E_INVALID_OPERATION:
    return "WBEM_E_INVALID_OPERATION";
  case WBEM_E_INVALID_QUERY:
    return "WBEM_E_INVALID_QUERY";
  case WBEM_E_INVALID_QUERY_TYPE:
    return "WBEM_E_INVALID_QUERY_TYPE";
  case WBEM_E_ALREADY_EXISTS:
    return "WBEM_E_ALREADY_EXISTS";
  case WBEM_E_OVERRIDE_NOT_ALLOWED:
    return "WBEM_E_OVERRIDE_NOT_ALLOWED";
  case WBEM_E_PROPAGATED_QUALIFIER:
    return "WBEM_E_PROPAGATED_QUALIFIER";
  case WBEM_E_PROPAGATED_PROPERTY:
    return "WBEM_E_PROPAGATED_PROPERTY";
  case WBEM_E_UNEXPECTED:
    return "WBEM_E_UNEXPECTED";
  case WBEM_E_ILLEGAL_OPERATION:
    return "WBEM_E_ILLEGAL_OPERATION";
  case WBEM_E_CANNOT_BE_KEY:
    return "WBEM_E_CANNOT_BE_KEY";
  case WBEM_E_INCOMPLETE_CLASS:
    return "WBEM_E_INCOMPLETE_CLASS";
  case WBEM_E_INVALID_SYNTAX:
    return "WBEM_E_INVALID_SYNTAX";
  case WBEM_E_NONDECORATED_OBJECT:
    return "WBEM_E_NONDECORATED_OBJECT";
  case WBEM_E_READ_ONLY:
    return "WBEM_E_READ_ONLY";
  case WBEM_E_PROVIDER_NOT_CAPABLE:
    return "WBEM_E_PROVIDER_NOT_CAPABLE";
  case WBEM_E_CLASS_HAS_CHILDREN:
    return "WBEM_E_CLASS_HAS_CHILDREN";
  case WBEM_E_CLASS_HAS_INSTANCES:
    return "WBEM_E_CLASS_HAS_INSTANCES";
  case WBEM_E_QUERY_NOT_IMPLEMENTED:
    return "WBEM_E_QUERY_NOT_IMPLEMENTED";
  case WBEM_E_ILLEGAL_NULL:
    return "WBEM_E_ILLEGAL_NULL";
  case WBEM_E_INVALID_QUALIFIER_TYPE:
    return "WBEM_E_INVALID_QUALIFIER_TYPE";
  case WBEM_E_INVALID_PROPERTY_TYPE:
    return "WBEM_E_INVALID_PROPERTY_TYPE";
  case WBEM_E_VALUE_OUT_OF_RANGE:
    return "WBEM_E_VALUE_OUT_OF_RANGE";
  case WBEM_E_CANNOT_BE_SINGLETON:
    return "WBEM_E_CANNOT_BE_SINGLETON";
  case WBEM_E_INVALID_CIM_TYPE:
    return "WBEM_E_INVALID_CIM_TYPE";
  case WBEM_E_INVALID_METHOD:
    return "WBEM_E_INVALID_METHOD";
  case WBEM_E_INVALID_METHOD_PARAMETERS:
    return "WBEM_E_INVALID_METHOD_PARAMETERS";
  case WBEM_E_SYSTEM_PROPERTY:
    return "WBEM_E_SYSTEM_PROPERTY";
  case WBEM_E_INVALID_PROPERTY:
    return "WBEM_E_INVALID_PROPERTY";
  case WBEM_E_CALL_CANCELLED:
    return "WBEM_E_CALL_CANCELLED";
  case WBEM_E_SHUTTING_DOWN:
    return "WBEM_E_SHUTTING_DOWN";
  case WBEM_E_PROPAGATED_METHOD:
    return "WBEM_E_PROPAGATED_METHOD";
  case WBEM_E_UNSUPPORTED_PARAMETER:
    return "WBEM_E_UNSUPPORTED_PARAMETER";
  case WBEM_E_MISSING_PARAMETER_ID:
    return "WBEM_E_MISSING_PARAMETER_ID";
  case WBEM_E_INVALID_PARAMETER_ID:
    return "WBEM_E_INVALID_PARAMETER_ID";
  case WBEM_E_NONCONSECUTIVE_PARAMETER_IDS:
    return "WBEM_E_NONCONSECUTIVE_PARAMETER_IDS";
  case WBEM_E_PARAMETER_ID_ON_RETVAL:
    return "WBEM_E_PARAMETER_ID_ON_RETVAL";
  case WBEM_E_INVALID_OBJECT_PATH:
    return "WBEM_E_INVALID_OBJECT_PATH";
  case WBEM_E_OUT_OF_DISK_SPACE:
    return "WBEM_E_OUT_OF_DISK_SPACE";
  case WBEM_E_BUFFER_TOO_SMALL:
    return "WBEM_E_BUFFER_TOO_SMALL";
  case WBEM_E_UNSUPPORTED_PUT_EXTENSION:
    return "WBEM_E_UNSUPPORTED_PUT_EXTENSION";
  case WBEM_E_UNKNOWN_OBJECT_TYPE:
    return "WBEM_E_UNKNOWN_OBJECT_TYPE";
  case WBEM_E_UNKNOWN_PACKET_TYPE:
    return "WBEM_E_UNKNOWN_PACKET_TYPE";
  case WBEM_E_MARSHAL_VERSION_MISMATCH:
    return "WBEM_E_MARSHAL_VERSION_MISMATCH";
  case WBEM_E_MARSHAL_INVALID_SIGNATURE:
    return "WBEM_E_MARSHAL_INVALID_SIGNATURE";
  case WBEM_E_INVALID_QUALIFIER:
    return "WBEM_E_INVALID_QUALIFIER";
  case WBEM_E_INVALID_DUPLICATE_PARAMETER:
    return "WBEM_E_INVALID_DUPLICATE_PARAMETER";
  case WBEM_E_TOO_MUCH_DATA:
    return "WBEM_E_TOO_MUCH_DATA";
  case WBEM_E_SERVER_TOO_BUSY:
    return "WBEM_E_SERVER_TOO_BUSY";
  case WBEM_E_INVALID_FLAVOR:
    return "WBEM_E_INVALID_FLAVOR";
  case WBEM_E_CIRCULAR_REFERENCE:
    return "WBEM_E_CIRCULAR_REFERENCE";
  case WBEM_E_UNSUPPORTED_CLASS_UPDATE:
    return "WBEM_E_UNSUPPORTED_CLASS_UPDATE";
  case WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE:
    return "WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE";
  case WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE:
    return "WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE";
  case WBEM_E_TOO_MANY_PROPERTIES:
    return "WBEM_E_TOO_MANY_PROPERTIES";
  case WBEM_E_UPDATE_TYPE_MISMATCH:
    return "WBEM_E_UPDATE_TYPE_MISMATCH";
  case WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED:
    return "WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED";
  case WBEM_E_UPDATE_PROPAGATED_METHOD:
    return "WBEM_E_UPDATE_PROPAGATED_METHOD";
  case WBEM_E_METHOD_NOT_IMPLEMENTED:
    return "WBEM_E_METHOD_NOT_IMPLEMENTED";
  case WBEM_E_METHOD_DISABLED:
    return "WBEM_E_METHOD_DISABLED";
  case WBEM_E_REFRESHER_BUSY:
    return "WBEM_E_REFRESHER_BUSY";
  case WBEM_E_UNPARSABLE_QUERY:
    return "WBEM_E_UNPARSABLE_QUERY";
  case WBEM_E_NOT_EVENT_CLASS:
    return "WBEM_E_NOT_EVENT_CLASS";
  case WBEM_E_MISSING_GROUP_WITHIN:
    return "WBEM_E_MISSING_GROUP_WITHIN";
  case WBEM_E_MISSING_AGGREGATION_LIST:
    return "WBEM_E_MISSING_AGGREGATION_LIST";
  case WBEM_E_PROPERTY_NOT_AN_OBJECT:
    return "WBEM_E_PROPERTY_NOT_AN_OBJECT";
  case WBEM_E_AGGREGATING_BY_OBJECT:
    return "WBEM_E_AGGREGATING_BY_OBJECT";
  case WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY:
    return "WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY";
  case WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING:
    return "WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING";
  case WBEM_E_QUEUE_OVERFLOW:
    return "WBEM_E_QUEUE_OVERFLOW";
  case WBEM_E_PRIVILEGE_NOT_HELD:
    return "WBEM_E_PRIVILEGE_NOT_HELD";
  case WBEM_E_INVALID_OPERATOR:
    return "WBEM_E_INVALID_OPERATOR";
  case WBEM_E_LOCAL_CREDENTIALS:
    return "WBEM_E_LOCAL_CREDENTIALS";
  case WBEM_E_CANNOT_BE_ABSTRACT:
    return "WBEM_E_CANNOT_BE_ABSTRACT";
  case WBEM_E_AMENDED_OBJECT:
    return "WBEM_E_AMENDED_OBJECT";
  case WBEM_E_CLIENT_TOO_SLOW:
    return "WBEM_E_CLIENT_TOO_SLOW";
  case WBEM_E_NULL_SECURITY_DESCRIPTOR:
    return "WBEM_E_NULL_SECURITY_DESCRIPTOR";
  case WBEM_E_TIMED_OUT:
    return "WBEM_E_TIMED_OUT";
  case WBEM_E_INVALID_ASSOCIATION:
    return "WBEM_E_INVALID_ASSOCIATION";
  case WBEM_E_AMBIGUOUS_OPERATION:
    return "WBEM_E_AMBIGUOUS_OPERATION";
  case WBEM_E_QUOTA_VIOLATION:
    return "WBEM_E_QUOTA_VIOLATION";
  case WBEM_E_RESERVED_001:
    return "WBEM_E_RESERVED_001";
  case WBEM_E_RESERVED_002:
    return "WBEM_E_RESERVED_002";
  case WBEM_E_UNSUPPORTED_LOCALE:
    return "WBEM_E_UNSUPPORTED_LOCALE";
  case WBEM_E_HANDLE_OUT_OF_DATE:
    return "WBEM_E_HANDLE_OUT_OF_DATE";
  case WBEM_E_CONNECTION_FAILED:
    return "WBEM_E_CONNECTION_FAILED";
  case WBEM_E_INVALID_HANDLE_REQUEST:
    return "WBEM_E_INVALID_HANDLE_REQUEST";
  case WBEM_E_PROPERTY_NAME_TOO_WIDE:
    return "WBEM_E_PROPERTY_NAME_TOO_WIDE";
  case WBEM_E_CLASS_NAME_TOO_WIDE:
    return "WBEM_E_CLASS_NAME_TOO_WIDE";
  case WBEM_E_METHOD_NAME_TOO_WIDE:
    return "WBEM_E_METHOD_NAME_TOO_WIDE";
  case WBEM_E_QUALIFIER_NAME_TOO_WIDE:
    return "WBEM_E_QUALIFIER_NAME_TOO_WIDE";
  case WBEM_E_RERUN_COMMAND:
    return "WBEM_E_RERUN_COMMAND";
  case WBEM_E_DATABASE_VER_MISMATCH:
    return "WBEM_E_DATABASE_VER_MISMATCH";
  case WBEM_E_VETO_DELETE:
    return "WBEM_E_VETO_DELETE";
  case WBEM_E_VETO_PUT:
    return "WBEM_E_VETO_PUT";
  case WBEM_E_INVALID_LOCALE:
    return "WBEM_E_INVALID_LOCALE";
  case WBEM_E_PROVIDER_SUSPENDED:
    return "WBEM_E_PROVIDER_SUSPENDED";
  case WBEM_E_SYNCHRONIZATION_REQUIRED:
    return "WBEM_E_SYNCHRONIZATION_REQUIRED";
  case WBEM_E_NO_SCHEMA:
    return "WBEM_E_NO_SCHEMA";
  case WBEM_E_PROVIDER_ALREADY_REGISTERED:
    return "WBEM_E_PROVIDER_ALREADY_REGISTERED";
  case WBEM_E_PROVIDER_NOT_REGISTERED:
    return "WBEM_E_PROVIDER_NOT_REGISTERED";
  case WBEM_E_FATAL_TRANSPORT_ERROR:
    return "WBEM_E_FATAL_TRANSPORT_ERROR";
  case WBEM_E_ENCRYPTED_CONNECTION_REQUIRED:
    return "WBEM_E_ENCRYPTED_CONNECTION_REQUIRED";
  case WBEM_E_PROVIDER_TIMED_OUT:
    return "WBEM_E_PROVIDER_TIMED_OUT";
  case WBEM_E_NO_KEY:
    return "WBEM_E_NO_KEY";
  case WBEM_E_PROVIDER_DISABLED:
    return "WBEM_E_PROVIDER_DISABLED";
  case WBEMESS_E_REGISTRATION_TOO_BROAD:
    return "WBEMESS_E_REGISTRATION_TOO_BROAD";
  case WBEMESS_E_REGISTRATION_TOO_PRECISE:
    return "WBEMESS_E_REGISTRATION_TOO_PRECISE";
//  case WBEMESS_E_AUTHZ_NOT_PRIVILEGED:
//    return "WBEMESS_E_AUTHZ_NOT_PRIVILEGED";
  case WBEMMOF_E_EXPECTED_QUALIFIER_NAME:
    return "WBEMMOF_E_EXPECTED_QUALIFIER_NAME";
  case WBEMMOF_E_EXPECTED_SEMI:
    return "WBEMMOF_E_EXPECTED_SEMI";
  case WBEMMOF_E_EXPECTED_OPEN_BRACE:
    return "WBEMMOF_E_EXPECTED_OPEN_BRACE";
  case WBEMMOF_E_EXPECTED_CLOSE_BRACE:
    return "WBEMMOF_E_EXPECTED_CLOSE_BRACE";
  case WBEMMOF_E_EXPECTED_CLOSE_BRACKET:
    return "WBEMMOF_E_EXPECTED_CLOSE_BRACKET";
  case WBEMMOF_E_EXPECTED_CLOSE_PAREN:
    return "WBEMMOF_E_EXPECTED_CLOSE_PAREN";
  case WBEMMOF_E_ILLEGAL_CONSTANT_VALUE:
    return "WBEMMOF_E_ILLEGAL_CONSTANT_VALUE";
  case WBEMMOF_E_EXPECTED_TYPE_IDENTIFIER:
    return "WBEMMOF_E_EXPECTED_TYPE_IDENTIFIER";
  case WBEMMOF_E_EXPECTED_OPEN_PAREN:
    return "WBEMMOF_E_EXPECTED_OPEN_PAREN";
  case WBEMMOF_E_UNRECOGNIZED_TOKEN:
    return "WBEMMOF_E_UNRECOGNIZED_TOKEN";
  case WBEMMOF_E_UNRECOGNIZED_TYPE:
    return "WBEMMOF_E_UNRECOGNIZED_TYPE";
  case WBEMMOF_E_EXPECTED_PROPERTY_NAME:
    return "WBEMMOF_E_EXPECTED_PROPERTY_NAME";
  case WBEMMOF_E_TYPEDEF_NOT_SUPPORTED:
    return "WBEMMOF_E_TYPEDEF_NOT_SUPPORTED";
  case WBEMMOF_E_UNEXPECTED_ALIAS:
    return "WBEMMOF_E_UNEXPECTED_ALIAS";
  case WBEMMOF_E_UNEXPECTED_ARRAY_INIT:
    return "WBEMMOF_E_UNEXPECTED_ARRAY_INIT";
  case WBEMMOF_E_INVALID_AMENDMENT_SYNTAX:
    return "WBEMMOF_E_INVALID_AMENDMENT_SYNTAX";
  case WBEMMOF_E_INVALID_DUPLICATE_AMENDMENT:
    return "WBEMMOF_E_INVALID_DUPLICATE_AMENDMENT";
  case WBEMMOF_E_INVALID_PRAGMA:
    return "WBEMMOF_E_INVALID_PRAGMA";
  case WBEMMOF_E_INVALID_NAMESPACE_SYNTAX:
    return "WBEMMOF_E_INVALID_NAMESPACE_SYNTAX";
  case WBEMMOF_E_EXPECTED_CLASS_NAME:
    return "WBEMMOF_E_EXPECTED_CLASS_NAME";
  case WBEMMOF_E_TYPE_MISMATCH:
    return "WBEMMOF_E_TYPE_MISMATCH";
  case WBEMMOF_E_EXPECTED_ALIAS_NAME:
    return "WBEMMOF_E_EXPECTED_ALIAS_NAME";
  case WBEMMOF_E_INVALID_CLASS_DECLARATION:
    return "WBEMMOF_E_INVALID_CLASS_DECLARATION";
  case WBEMMOF_E_INVALID_INSTANCE_DECLARATION:
    return "WBEMMOF_E_INVALID_INSTANCE_DECLARATION";
  case WBEMMOF_E_EXPECTED_DOLLAR:
    return "WBEMMOF_E_EXPECTED_DOLLAR";
  case WBEMMOF_E_CIMTYPE_QUALIFIER:
    return "WBEMMOF_E_CIMTYPE_QUALIFIER";
  case WBEMMOF_E_DUPLICATE_PROPERTY:
    return "WBEMMOF_E_DUPLICATE_PROPERTY";
  case WBEMMOF_E_INVALID_NAMESPACE_SPECIFICATION:
    return "WBEMMOF_E_INVALID_NAMESPACE_SPECIFICATION";
  case WBEMMOF_E_OUT_OF_RANGE:
    return "WBEMMOF_E_OUT_OF_RANGE";
  case WBEMMOF_E_INVALID_FILE:
    return "WBEMMOF_E_INVALID_FILE";
  case WBEMMOF_E_ALIASES_IN_EMBEDDED:
    return "WBEMMOF_E_ALIASES_IN_EMBEDDED";
  case WBEMMOF_E_NULL_ARRAY_ELEM:
    return "WBEMMOF_E_NULL_ARRAY_ELEM";
  case WBEMMOF_E_DUPLICATE_QUALIFIER:
    return "WBEMMOF_E_DUPLICATE_QUALIFIER";
  case WBEMMOF_E_EXPECTED_FLAVOR_TYPE:
    return "WBEMMOF_E_EXPECTED_FLAVOR_TYPE";
  case WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES:
    return "WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES";
  case WBEMMOF_E_MULTIPLE_ALIASES:
    return "WBEMMOF_E_MULTIPLE_ALIASES";
  case WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2:
    return "WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2";
  case WBEMMOF_E_NO_ARRAYS_RETURNED:
    return "WBEMMOF_E_NO_ARRAYS_RETURNED";
  case WBEMMOF_E_MUST_BE_IN_OR_OUT:
    return "WBEMMOF_E_MUST_BE_IN_OR_OUT";
  case WBEMMOF_E_INVALID_FLAGS_SYNTAX:
    return "WBEMMOF_E_INVALID_FLAGS_SYNTAX";
  case WBEMMOF_E_EXPECTED_BRACE_OR_BAD_TYPE:
    return "WBEMMOF_E_EXPECTED_BRACE_OR_BAD_TYPE";
  case WBEMMOF_E_UNSUPPORTED_CIMV22_QUAL_VALUE:
    return "WBEMMOF_E_UNSUPPORTED_CIMV22_QUAL_VALUE";
  case WBEMMOF_E_UNSUPPORTED_CIMV22_DATA_TYPE:
    return "WBEMMOF_E_UNSUPPORTED_CIMV22_DATA_TYPE";
  case WBEMMOF_E_INVALID_DELETEINSTANCE_SYNTAX:
    return "WBEMMOF_E_INVALID_DELETEINSTANCE_SYNTAX";
  case WBEMMOF_E_INVALID_QUALIFIER_SYNTAX:
    return "WBEMMOF_E_INVALID_QUALIFIER_SYNTAX";
  case WBEMMOF_E_QUALIFIER_USED_OUTSIDE_SCOPE:
    return "WBEMMOF_E_QUALIFIER_USED_OUTSIDE_SCOPE";
  case WBEMMOF_E_ERROR_CREATING_TEMP_FILE:
    return "WBEMMOF_E_ERROR_CREATING_TEMP_FILE";
  case WBEMMOF_E_ERROR_INVALID_INCLUDE_FILE:
    return "WBEMMOF_E_ERROR_INVALID_INCLUDE_FILE";
  case WBEMMOF_E_INVALID_DELETECLASS_SYNTAX:
    return "WBEMMOF_E_INVALID_DELETECLASS_SYNTAX";
  case STRSAFE_E_INVALID_PARAMETER:
    return "STRSAFE_E_INVALID_PARAMETER";
  case E_ACCESSDENIED:
    return "E_ACCESSDENIED";
    default:
      sfsprintf (v2sbuf, sizeof (v2sbuf), "Unknown error: 0x%x\n", hr);
    return v2sbuf;
  }
  return "";
}

const char *VT2ccp (int t) {
  switch (t) {
  case VT_UI1:               return "VT_UI1";
  case VT_I2:                return "VT_I2";
  case VT_I4:                return "VT_I4";
  case VT_R4:                return "VT_R4";
  case VT_R8:                return "VT_R8";
  case VT_BOOL:              return "VT_BOOL";
  case VT_ERROR:             return "VT_ERROR";
  case VT_CY:                return "VT_CY";
  case VT_DATE:              return "VT_DATE";
  case VT_BSTR:              return "VT_BSTR";
  case VT_UNKNOWN:           return "VT_UNKNOWN";
  case VT_DISPATCH:          return "VT_DISPATCH";
  case VT_BOOL|VT_ARRAY:     return "VT_BOOL|VT_ARRAY";
  case VT_UI1|VT_ARRAY:      return "VT_UI1|VT_ARRAY";
  case VT_I2|VT_ARRAY:       return "VT_I2|VT_ARRAY";
  case VT_I4|VT_ARRAY:       return "VT_I4|VT_ARRAY";
  case VT_R4|VT_ARRAY:       return "VT_R4|VT_ARRAY";
  case VT_R8|VT_ARRAY:       return "VT_R8|VT_ARRAY";
  case VT_BSTR|VT_ARRAY:     return "VT_BSTR|VT_ARRAY";
  case VT_DISPATCH|VT_ARRAY: return "VT_DISPATCH|VT_ARRAY";
  case VT_BYREF|VT_UI1:      return "VT_BYREF|VT_UI1";
  case VT_BYREF|VT_I2:       return "VT_BYREF|VT_I2";
  case VT_BYREF|VT_I4:       return "VT_BYREF|VT_I4";
  case VT_BYREF|VT_R4:       return "VT_BYREF|VT_R4";
  case VT_BYREF|VT_R8:       return "VT_BYREF|VT_R8";
  case VT_BYREF|VT_BOOL:     return "VT_BYREF|VT_BOOL";
  case VT_BYREF|VT_ERROR:    return "VT_BYREF|VT_ERROR";
  case VT_BYREF|VT_CY:       return "VT_BYREF|VT_CY";
  case VT_BYREF|VT_DATE:     return "VT_BYREF|VT_DATE";
  case VT_BYREF|VT_BSTR:     return "VT_BYREF|VT_BSTR";
  case VT_BYREF|VT_UNKNOWN:  return "VT_BYREF|VT_DISPATCH";
  case VT_BYREF|VT_DISPATCH: return "VT_BYREF|VT_DISPATCH";
  case VT_BYREF|VT_VARIANT:  return "VT_BYREF|VT_VARIANT";
  default:                   return "VARIANT TYPE ERROR";
  }
}

const char *V2ccp (VARIANT *pv) {
  HRESULT hr;
  const char *sep = ":";

  switch (pv->vt) {
  case VT_NULL: return "NULL";
  case VT_BOOL:
    if (!pv->boolVal) return "FALSE";
    else              return "TRUE";

  case VT_UI1:
    sfsprintf (v2sbuf, sizeof (v2sbuf), "%d", (int) pv->bVal);
    break;
  case VT_I2:
    sfsprintf (v2sbuf, sizeof (v2sbuf), "%d", (int) pv->iVal);
    break;
  case VT_I4:
    sfsprintf (v2sbuf, sizeof (v2sbuf), "%d", (int) pv->lVal);
    break;
  case VT_R4:
    sfsprintf (v2sbuf, sizeof (v2sbuf), "%f", pv->fltVal);
    break;
  case VT_R8:
    sfsprintf (v2sbuf, sizeof (v2sbuf), "%lf", pv->dblVal);
    break;
  case VT_BSTR: return b2s (pv->bstrVal);
  case VT_ARRAY|VT_UI1:
  case VT_ARRAY|VT_I2:
  case VT_ARRAY|VT_I4:
  case VT_ARRAY|VT_R4:
  case VT_ARRAY|VT_R8: {
    long l, u, i;
    if ((hr = SafeArrayGetLBound (pv->parray, 1, &l)) != S_OK) {
      sfprintf (
        sfstderr, "V2ccp(SafeArrayGetLBound()) failed: %s\n", E2ccp (hr)
      );
      return "";
    }
    if ((hr = SafeArrayGetUBound (pv->parray, 1, &u)) != S_OK) {
      sfprintf (
        sfstderr, "V2ccp(SafeArrayGetUBound()) failed: %s\n", E2ccp (hr)
      );
      return "";
    }
    if (u == l) {
//      sfprintf (sfstderr, "V2ccp: no elements 1\n");
      return "";
    }
    for (i = l; i <= u; i++) {
      int b;
      if ((hr = SafeArrayGetElement (pv->parray, &i, &b)) != S_OK) {
        sfprintf (
          sfstderr, "V2ccp(SafeArrayGetElement()) failed: %s\n", E2ccp (hr)
        );
        break;
      }
      sfsprintf (v2sbuf, sizeof (v2sbuf), "%s%d", (i > l) ? sep : "", b);
    }
    break;
  }
  case VT_ARRAY|VT_BSTR: {
    long l, u, i;
    if ((hr = SafeArrayGetLBound (pv->parray, 1, &l)) != S_OK) {
      sfprintf (
        sfstderr, "V2ccp(SafeArrayGetLBound()) failed: %s\n", E2ccp (hr)
      );
      return "";
    }
    if ((hr = SafeArrayGetUBound (pv->parray, 1, &u)) != S_OK) {
      sfprintf (
        sfstderr, "V2ccp(SafeArrayGetUBound()) failed: %s\n", E2ccp (hr)
      );
      return "";
    }
    if (u == l) {
//      sfprintf (sfstderr, "V2ccp: no elements 2\n");
      return "";
    }
    for (i = l; i <= u; i++) {
      BSTR b;
      if ((hr = SafeArrayGetElement (pv->parray, &i, &b)) != S_OK) {
        sfprintf (
          sfstderr, "V2ccp(SafeArrayGetElement()) failed: %s\n", E2ccp (hr)
        );
        break;
      }
      sfsprintf (v2sbuf, sizeof (v2sbuf), "%s%s", (i > l) ? sep : "", b2s (b));
      ::SysFreeString (b);
    }
    break;
  }
  default:
    sfprintf (sfstderr, "V2ccp: %s\n", VT2ccp (pv->vt));
    return "Unsupported";
  }
  return v2sbuf;
}
