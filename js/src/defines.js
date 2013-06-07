/**
 * Constants
 */
var HV_NONE = 0;
var HV_VIRTUALBOX = 1;
var HVE_ALREADY_EXISTS = 2;
var HVE_SCHEDULED = 1;
var HVE_OK = 0;
var HVE_CREATE_ERROR = -1;
var HVE_MODIFY_ERROR = -2;
var HVE_CONTROL_ERROR = -3;
var HVE_DELETE_ERROR = -4;
var HVE_QUERY_ERROR = -5;
var HVE_IO_ERROR = -6;
var HVE_EXTERNAL_ERROR = -7;
var HVE_INVALID_STATE = -8;
var HVE_NOT_FOUND = -9;
var HVE_NOT_ALLOWED = -10;
var HVE_NOT_SUPPORTED = -11;
var HVE_NOT_VALIDATED = -12;
var HVE_NOT_TRUSTED = -13;
var HVE_USAGE_ERROR = -99;
var HVE_NOT_IMPLEMENTED = -100;

var STATE_CLOSED = 0;
var STATE_OPPENING = 1;
var STATE_OPEN = 2;
var STATE_STARTING = 3;
var STATE_STARTED = 4;
var STATE_ERROR = 5;
var STATE_PAUSED = 6;

var HVF_SYSTEM_64BIT = 1; 
var HVF_DEPLOYMENT_HDD = 2; 
var HVF_GUEST_ADDITIONS = 4;
var HVF_FLOPPY_IO = 8;