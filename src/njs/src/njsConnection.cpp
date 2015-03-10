/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved. */

/******************************************************************************
 *
 * You may not use the identified files except in compliance with the Apache 
 * License, Version 2.0 (the "License.")
 *
 * You may obtain a copy of the License at 
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 *
 * NAME
 *   njsConnection.cpp
 *
 * DESCRIPTION
 *   Connection class implementation.
 *
 *****************************************************************************/

#include "njsConnection.h"
#include <stdlib.h>
#include <iostream>
using namespace std;
                                        //peristent Connection class handle
Persistent<FunctionTemplate> Connection::connectionTemplate_s;

#define NJS_MAX_OUT_BIND_SIZE 200
// # of milliseconds in a day.  Used to convert from/to v8::Date to Oracle/Date
#define NJS_DAY2MS       (24.0 * 60.0 * 60.0 * 1000.0 )

/*****************************************************************************/
/*
   DESCRIPTION
     Constructor for the Connection class.
 */
Connection::Connection()
{
   dpiconn_   = (dpi::Conn *)0;
   oracledb_  = (Oracledb *)0;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Destructor for the Connection class.
 */
Connection::~Connection() 
{}

/*****************************************************************************/
/*
   DESCRIPTION
     Initialize connection attributes after forming it.
  
   PARAMETERS:
     DPI Connection, Oracledb reference 
*/
void Connection::setConnection(dpi::Conn* dpiconn, Oracledb* oracledb)
{
   this->dpiconn_  = dpiconn;
   this->isValid_  = true;
   this->oracledb_ = oracledb;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Init function of the Connection class.
     Initiates and maps the functions and properties of Connection class.
*/     
void Connection::Init(Handle<Object> target)
{
  NanScope();

  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(NanNew<v8::String>("Connection"));

  NODE_SET_PROTOTYPE_METHOD(tpl, "execute", Execute);
  NODE_SET_PROTOTYPE_METHOD(tpl, "release", Release);
  NODE_SET_PROTOTYPE_METHOD(tpl, "commit", Commit);
  NODE_SET_PROTOTYPE_METHOD(tpl, "rollback", Rollback);
  NODE_SET_PROTOTYPE_METHOD(tpl, "break", Break);

  tpl->InstanceTemplate()->SetAccessor(
                                              NanNew<v8::String>("stmtCacheSize"),
                                              Connection::GetStmtCacheSize,
                                              Connection::SetStmtCacheSize ); 
  tpl->InstanceTemplate()->SetAccessor(
                                              NanNew<v8::String>("clientId"),
                                              Connection::GetClientId,
                                              Connection::SetClientId ); 
  tpl->InstanceTemplate()->SetAccessor(
                                              NanNew<v8::String>("module"),
                                              Connection::GetModule,
                                              Connection::SetModule ); 
  tpl->InstanceTemplate()->SetAccessor(
                                              NanNew<v8::String>("action"),
                                              Connection::GetAction,
                                              Connection::SetAction ); 

  NanAssignPersistent( connectionTemplate_s, tpl);
  target->Set(NanNew<v8::String>("Connection"),tpl->GetFunction());
}

/*****************************************************************************/
/*
   DESCRIPTION
     Invoked when new of connection is called from JS
*/
NAN_METHOD(Connection::New)
{
  NanScope();

  Connection *connection = new Connection();
  connection->Wrap(args.This());

  NanReturnValue(args.This());
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of stmtCacheSize property
*/
NAN_GETTER(Connection::GetStmtCacheSize)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder()); 
  if(!njsConn->isValid_)
  {
    string error = NJSMessages::getErrorMsg ( errInvalidPool );
    NJS_SET_EXCEPTION(error.c_str(), error.length());
    NanReturnUndefined();
  }
  try
  {
    Local<Integer> value = NanNew<v8::Integer>(njsConn->dpiconn_->stmtCacheSize());
    NanReturnValue(value);
  }
  catch(dpi::Exception &e)
  {
    NJS_SET_EXCEPTION(e.what(), strlen(e.what()));
    NanReturnUndefined();
  }
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of stmtCacheSize property - throws error
*/
NAN_SETTER(Connection::SetStmtCacheSize)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder()); 
  string msg;
  if(!njsConn->isValid_)
    msg = NJSMessages::getErrorMsg(errInvalidConnection);
  else
    msg = NJSMessages::getErrorMsg(errReadOnly, "stmtCacheSize");
  NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of clientId property - throws error
*/
NAN_GETTER(Connection::GetClientId)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder()); 
  string msg;
  if(!njsConn->isValid_)
    msg = NJSMessages::getErrorMsg(errInvalidConnection);
  else
    msg = NJSMessages::getErrorMsg(errWriteOnly, "clientId");
  NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of clientId property
*/
NAN_SETTER(Connection::SetClientId)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder());
  if(!njsConn->isValid_)
  {
    string msg = NJSMessages::getErrorMsg(errInvalidConnection);
    NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
    return;
  }
  else
  {
    std::string client;
    NJS_SET_PROP_STR(client, value, "clientId");
    njsConn->dpiconn_->clientId(client);
  }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of module property - throws error
*/
NAN_GETTER(Connection::GetModule)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder()); 
  string msg;
  if(!njsConn->isValid_)
    msg = NJSMessages::getErrorMsg(errInvalidConnection);
  else
    msg = NJSMessages::getErrorMsg(errWriteOnly, "module");
  NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of stmtCacheSize property
*/
NAN_SETTER(Connection::SetModule)
{
  NanScope();
  Connection *njsConn = ObjectWrap::Unwrap<Connection>(args.Holder());
  if(!njsConn->isValid_)
  {
    string msg = NJSMessages::getErrorMsg(errInvalidConnection);
    NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
    return;
  }
  else
  {
    std::string module;
    NJS_SET_PROP_STR( module, value, "module");
    njsConn->dpiconn_->module(module);
  }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of action property - throws error
*/
NAN_GETTER(Connection::GetAction)
{
  NanScope();
  Connection* njsConn = ObjectWrap::Unwrap<Connection>(args.Holder()); 
  string msg;
  if(!njsConn->isValid_)
    msg = NJSMessages::getErrorMsg(errInvalidConnection);
  else
    msg = NJSMessages::getErrorMsg(errWriteOnly, "action");
  NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of action property
*/
NAN_SETTER(Connection::SetAction)
{
  NanScope();
  Connection *njsConn = ObjectWrap::Unwrap<Connection>(args.Holder());
  if(!njsConn->isValid_)
  {
    string msg = NJSMessages::getErrorMsg(errInvalidConnection);
    NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
    return;
  }
  else
  {
    std::string action;
    NJS_SET_PROP_STR( action, value, "action");
    njsConn->dpiconn_->action(action);
  }
}
/*****************************************************************************/
/*
   DESCRIPTION
     Execute method on Connection class.
  
   PARAMETERS:
     Arguments - SQL Statement,
                 Binds Object/Array (Optional),
                 Options Object (Optional),
                 Callback
*/
NAN_METHOD(Connection::Execute)
{
  NanScope();
  Local<Function> callback;
  Local<String> sql;
  Connection *connection;
  NJS_GET_CALLBACK ( callback, args );

  eBaton *executeBaton = new eBaton;
  NanAssignPersistent( executeBaton->cb, callback ); 
  NJS_CHECK_NUMBER_OF_ARGS ( executeBaton->error, args, 2, 4, exitExecute );
  connection = ObjectWrap::Unwrap<Connection>(args.This());

  if(!connection->isValid_)
  {
    executeBaton->error = NJSMessages::getErrorMsg ( errInvalidConnection );
    goto exitExecute;
  }
  NJS_GET_ARG_V8STRING (sql, executeBaton->error, args, 0, exitExecute);
  NJSString (executeBaton->sql, sql);

  executeBaton->maxRows      = connection->oracledb_->getMaxRows();
  executeBaton->outFormat    = connection->oracledb_->getOutFormat();
  executeBaton->isAutoCommit = connection->oracledb_->getIsAutoCommit();
  executeBaton->dpienv       = connection->oracledb_->getDpiEnv();
  executeBaton->dpiconn      = connection->dpiconn_;
 
  if(args.Length() > 2)
  {
    Connection::ProcessBinds(args, 1, executeBaton);
    if(!executeBaton->error.empty()) goto exitExecute;
  }
  if(args.Length() > 3)
  {
    Connection::ProcessOptions(args, 2, executeBaton);
     if(!executeBaton->error.empty()) goto exitExecute;
  }

  exitExecute:
  executeBaton->req.data  = (void*) executeBaton;
  uv_queue_work(uv_default_loop(), &executeBaton->req, 
               Async_Execute, (uv_after_work_cb)Async_AfterExecute);

  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing of Binds 

   PARAMETERS:
     args - Arguments to execute call,
     index- index of binds in args, 
     executeBaton 
 */
void Connection::ProcessBinds (_NAN_METHOD_ARGS, unsigned int index,
                               eBaton* executeBaton)
{
  NanScope();
  if(args[index]->IsArray() )
  {
    Local<Array> bindsArray  = Local<Array>::Cast(args[index]);
    Connection::GetBinds( bindsArray ,executeBaton );
  }
  else if(args[index]->IsObject() && !args[index]->IsFunction())
  {
    Local<Object> bindsObject  = args[index]->ToObject();
    Connection::GetBinds( bindsObject, executeBaton);
  }
  else
    executeBaton->error = NJSMessages::getErrorMsg(errInvalidParameterType, index);
}
 
/*****************************************************************************/
/*
   DESCRIPTION
     Processing of options

   PARAMETERS:
     args - Arguments to execute call,
     index- index of options in args, 
     executeBaton 
 */
void Connection::ProcessOptions (_NAN_METHOD_ARGS, unsigned int index,
                                 eBaton* executeBaton)
{
  Local<Object> options;
  if(args[index]->IsObject() && !args[index]->IsArray())
  {
    options = args[index]->ToObject();
    NJS_GET_UINT_FROM_JSON   ( executeBaton->maxRows, executeBaton->error,
                               options, "maxRows", 2, exitProcessOptions );
    NJS_GET_UINT_FROM_JSON   ( executeBaton->outFormat, executeBaton->error,
                               options, "outFormat", 2, exitProcessOptions );
    NJS_GET_BOOL_FROM_JSON   ( executeBaton->isAutoCommit, executeBaton->error,
                               options, "isAutoCommit", 2, exitProcessOptions );
  }
  else 
  {
    executeBaton->error = NJSMessages::getErrorMsg(errInvalidParameterType, index);
    goto exitProcessOptions;
  }
  exitProcessOptions:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing of Binds Object 
  
   PARAMETERS:
     Handle Object, eBaton struct 
  
   NOTES:
     Overloaded function
*/
void Connection::GetBinds (Handle<Object> bindobj, eBaton* executeBaton)
{
  NanScope();
  std::string str;
  Local<Array> array = bindobj->GetOwnPropertyNames();

  for(unsigned int index=0; index<array->Length(); index++)
  {
    Bind* bind = new Bind;
    Handle<String> temp = array->Get(index).As<String>();
    NJSString(str, temp);
    bind->key = ":"+std::string(str);
    Handle<Value> val__ = bindobj->Get(NanNew<v8::String>((char*)str.c_str(),
						   (int) str.length()));
    Connection::GetBindUnit(val__, bind, executeBaton);
    if(!executeBaton->error.empty())
      goto exitGetBinds;
  }
  exitGetBinds:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing of Binds Array
   
   PARAMETERS:
     Handle Array, eBaton struct 
  
   NOTES:
     Overloaded function
*/
void Connection::GetBinds (Handle<Array> binds, eBaton* executeBaton)
{
  NanScope();
 
  for(unsigned int index = 0; index < binds->Length(); index++)
  {
    Bind* bind = new Bind;  
    Local<Value> val__ = binds->Get(index);
    GetBindUnit(val__, bind, executeBaton);
    if(!executeBaton->error.empty()) goto exitGetBinds;
  }
  exitGetBinds:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing each bind varible 
  
   PARAMETERS:
     Handle value, eBaton struct 
*/
void Connection::GetBindUnit (Handle<Value> val, Bind* bind,
                                       eBaton* executeBaton)
{
  NanScope();
  unsigned int dir   = BIND_IN;

  if(val->IsObject() && !val->IsDate())
  {
    Local<Object> bind_unit = val->ToObject();
    NJS_GET_UINT_FROM_JSON   ( dir, executeBaton->error, 
                               bind_unit, "dir", 1, exitGetBindUnit );
    NJS_GET_UINT_FROM_JSON   ( bind->type, executeBaton->error, 
                               bind_unit, "type", 1, exitGetBindUnit );
    bind->maxSize = NJS_MAX_OUT_BIND_SIZE;
    NJS_GET_UINT_FROM_JSON   ( bind->maxSize, executeBaton->error, 
                               bind_unit, "maxSize", 1, exitGetBindUnit );
    if(!bind->maxSize && dir != BIND_IN)
    {
      executeBaton->error = NJSMessages::getErrorMsg (
                                           errInvalidPropertyValueInParam,
                                           "maxSize", 2 );
      goto exitGetBindUnit;
    }

    Local<Value> element = bind_unit->Get(NanNew<v8::String>("val"));
    switch(dir)
    {
      case BIND_IN    :
        bind->isOut  = false;
        Connection::GetInBindParams(element, bind, executeBaton, BIND_IN );
        if(!executeBaton->error.empty()) goto exitGetBindUnit;
        break;
      case BIND_INOUT :
        bind->isOut  = true;
        Connection::GetInBindParams(element, bind, executeBaton, BIND_INOUT);
        if(!executeBaton->error.empty()) goto exitGetBindUnit;
        break;
      case BIND_OUT   :
        bind->isOut  = true;
        bind->ind  = 0;
        Connection::GetOutBindParams(bind->type, bind, executeBaton);
        if(!executeBaton->error.empty()) goto exitGetBindUnit;
        break;
      default         :
        executeBaton->error = NJSMessages::getErrorMsg (errInvalidBindDirection); 
        goto exitGetBindUnit;
        break;
    }
  }
  else
  {
    bind->isOut  = false;
    Connection::GetInBindParams(val, bind, executeBaton, BIND_IN);
    if(!executeBaton->error.empty()) goto exitGetBindUnit;
  }
  exitGetBindUnit:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing out binds
  
   PARAMETERS:
     dataType - datatype of the bind
     bind struct, eBaton struct 
*/
void Connection::GetOutBindParams (unsigned short dataType, Bind* bind,
                                   eBaton *executeBaton)
{
  NanScope();
  switch(dataType)
  {
    case DATA_STR :
      bind->type =  dpi::DpiVarChar;
      bind->value = (char*)malloc(bind->maxSize);
      break;
    case DATA_NUM :
      bind->type = dpi::DpiDouble;
      bind->maxSize = sizeof(double);
      bind->value = (double*)malloc(bind->maxSize); 
      break;
    case DATA_DATE :
      bind->extvalue = (long double *) malloc ( sizeof ( long double ) );
      bind->type  = dpi::DpiTimestampLTZ;
      bind->maxSize = 0;
      break;
    default :
      executeBaton->error= NJSMessages::getErrorMsg(errInvalidBindDataType,2);
      break;
  }
  executeBaton->binds.push_back(bind);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Processing in binds 
  
   PARAMETERS:
     Handle value, bind struct, eBaton struct 
*/
void Connection::GetInBindParams (Handle<Value> v8val, Bind* bind,
                                           eBaton* executeBaton, BindType type)
{
  bind->ind  = 0;
  if(v8val->IsUndefined() || v8val->IsNull())
  {
    bind->value = NULL;
    bind->ind   = -1;
    bind->type        = dpi::DpiVarChar; 
  }
  else if(v8val->IsString())
  {
    if( bind->type && bind->type != DATA_STR )
    {
      executeBaton->error= NJSMessages::getErrorMsg(
                             errBindValueAndTypeMismatch, 2);
      goto exitGetInBindParams;
    }

    v8::String::Utf8Value str(v8val->ToString());
  
    bind->type = dpi::DpiVarChar;
    if(type == BIND_INOUT)
    {  
      bind->len = str.length();
    }
    else // IN
    {
      bind->maxSize = bind->len = str.length();
    }
    DPI_SZ_TYPE size = (bind->maxSize >= bind->len ) ?
                        bind->maxSize : bind->len;
    if(size)
    {    
      bind->value = (char*)malloc(size);
      if(str.length())
        memcpy(bind->value, *str, str.length());
    }    
  }
  else if(v8val->IsInt32())
  {
    if( bind->type && bind->type != DATA_NUM )
    {
      executeBaton->error= NJSMessages::getErrorMsg(
                             errBindValueAndTypeMismatch, 2);
      goto exitGetInBindParams;
    }
    bind->type = dpi::DpiInteger;
    bind->maxSize = bind->len = sizeof(int);
    bind->value = (int*)malloc(bind->len); 
    *(int*)(bind->value) = v8val->ToInt32()->Value();
  }
  else if(v8val->IsUint32())
  {
    if( bind->type && bind->type != DATA_NUM )
    {
      executeBaton->error= NJSMessages::getErrorMsg(
                             errBindValueAndTypeMismatch, 2);
      goto exitGetInBindParams;
    }
    bind->type = dpi::DpiUnsignedInteger;
    bind->maxSize = bind->len = sizeof(unsigned int);
    bind->value = (unsigned int*)malloc(bind->len); 
    *(unsigned int*)(bind->value) = v8val->ToUint32()->Value();
  }
  else if(v8val->IsNumber())
  {
    if( bind->type && bind->type != DATA_NUM )
    {
      executeBaton->error= NJSMessages::getErrorMsg(errBindValueAndTypeMismatch, 2);
      goto exitGetInBindParams;
    }
    bind->type = dpi::DpiDouble;
    bind->maxSize = bind->len = sizeof(double);
    bind->value = (double*)malloc(bind->len); 
    *(double*)(bind->value) = v8val->NumberValue();
  }
  else if(v8val->IsDate ())
  {
    if( bind->type && bind->type != DATA_DATE )
    {
      executeBaton->error= NJSMessages::getErrorMsg(errBindValueAndTypeMismatch, 2);
      goto exitGetInBindParams;
    }
    /* This has to be allocated after stmt is initialized */
    bind->dttmarr = NULL ;
    bind->extvalue = (long double *) malloc (sizeof ( long double ) );
    bind->value = (long double *)malloc (sizeof ( long double ));
    bind->type = dpi::DpiTimestampLTZ;
    bind->len = 0;
    bind->maxSize = 0;
    /* Convert v8::Date value to long double */
    Connection::v8Date2OraDate ( v8val, bind);
  }
  else
  {
    executeBaton->error= NJSMessages::getErrorMsg(errInvalidBindDataType,2);
    goto exitGetInBindParams;
  }
  executeBaton->binds.push_back(bind);
  exitGetInBindParams:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Execute  method
  
   PARAMETERS:
     UV queue work block 

   NOTES:
     DPI call execution
*/
void Connection::Async_Execute (uv_work_t *req)
{
  eBaton *executeBaton = (eBaton*)req->data;
  if(!(executeBaton->error).empty()) goto exitAsyncExecute;
  
  try 
  { 
    Connection::PrepareAndBind(executeBaton); 
    executeBaton->st = executeBaton->dpistmt->stmtType();
    if (executeBaton->st == DpiStmtSelect)
    {
      executeBaton->dpistmt->executeQuery();
      Connection::GetDefines(executeBaton);
    }
    else
    {
      executeBaton->dpistmt->executeDML(executeBaton->isAutoCommit);
      executeBaton->rowsAffected = executeBaton->dpistmt->rowsAffected();

     /* Process date/timestamp INOUT/OUT bind values */
     for ( unsigned int b = 0; b < executeBaton->binds.size (); b++ )
     {
       /* Interested only OUT binds of date/timestamp type */
       if ( executeBaton->binds[b]->isOut && executeBaton->binds[b]->dttmarr)
       {
         *(long double *)(executeBaton->binds[b]->extvalue) =
           executeBaton->binds[b]->dttmarr->getDateTime ( 0 ) * NJS_DAY2MS ;
       }

       /* DATE/Timestamp could have been allocated for IN/OUT/INOUT binds */
       if ( executeBaton->binds[b]->dttmarr )
       {
         executeBaton->binds[b]->dttmarr->release () ;
         executeBaton->binds[b]->dttmarr = NULL;
       }
     }
    }
    if ( executeBaton->dpistmt )
    {
      executeBaton->dpistmt->release ();
    }   
  }
  catch (dpi::Exception& e)
  {
      executeBaton->error = std::string(e.what());
  }
  exitAsyncExecute:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Calls DPI Stmt Prepare and Bind

   PARAMETERS:
     eBaton struct
 */
void Connection::PrepareAndBind (eBaton* executeBaton)
{
    executeBaton->dpistmt = executeBaton->dpiconn->getStmt(executeBaton->sql);
    if(!executeBaton->binds.empty())
    {
      if(!executeBaton->binds[0]->key.empty())
      {
        for(unsigned int index = 0 ;index < executeBaton->binds.size();
            index++) 
        {
          // Convert v8::Date to Oracle DB Type
          if ( executeBaton->binds[index]->type == DpiTimestampLTZ )
          {
            Connection::UpdateDateValue ( executeBaton ) ;
          }
          // Bind by name 
          executeBaton->dpistmt->bind((const unsigned char*)executeBaton->binds[index]->key.c_str(),
                             (int) executeBaton->binds[index]->key.length(),
                             executeBaton->binds[index]->type,
                             executeBaton->binds[index]->value,
                             executeBaton->binds[index]->maxSize,
                             &executeBaton->binds[index]->ind,
                             &executeBaton->binds[index]->len);
        }
      }
      else
      {
        for(unsigned int index = 0 ;index < executeBaton->binds.size(); index++) 
        {
          if ( executeBaton->binds[index]->type == DpiTimestampLTZ )
          {
            Connection::UpdateDateValue ( executeBaton ) ;
          }
          // Bind by position 
          executeBaton->dpistmt->bind(index+1,executeBaton->binds[index]->type,
                             executeBaton->binds[index]->value,
                             executeBaton->binds[index]->maxSize,
                             &executeBaton->binds[index]->ind,
                             &executeBaton->binds[index]->len);
        }
      }
    }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Allocate defines buffer for query output. 
     Call DPI define and fetch.

   PARAMETERS:
     eBaton struct
 */
void Connection::GetDefines (eBaton* executeBaton)
{
  unsigned int     numCols   = executeBaton->dpistmt->numCols();
  Define *defines            = new Define[numCols];
  const dpi::MetaData* meta  = executeBaton->dpistmt->getMetaData();
  executeBaton->columnNames  = new std::string[numCols];  

  for (unsigned int i = 0; i < numCols; i++)
  {

    executeBaton->columnNames[i] = std::string((const char*)meta[i].colName,
                                               meta[i].colNameLen );

    switch(meta[i].dbType)
    {
      case dpi::DpiNumber :
      case dpi::DpiBinaryFloat :
      case dpi::DpiBinaryDouble :
        defines[i].fetchType = dpi::DpiDouble;
        defines[i].maxSize   = sizeof(double); 
        defines[i].buf = (double *)malloc(defines[i].maxSize*executeBaton->maxRows);
        break;
      case dpi::DpiVarChar :
      case dpi::DpiFixedChar :
        defines[i].fetchType = DpiVarChar;
        defines[i].maxSize   = meta[i].dbSize; 
        defines[i].buf = (char *)malloc(defines[i].maxSize*executeBaton->maxRows);
        break;
      case dpi::DpiDate :
      case dpi::DpiTimestamp:
      case dpi::DpiTimestampLTZ:
        defines[i].dttmarr   = executeBaton->dpienv->getDateTimeArray ( 
                                     executeBaton->dpistmt->getError () );
        defines[i].fetchType = DpiTimestampLTZ;
        defines[i].maxSize   = meta[i].dbSize;
        defines[i].extbuf    = defines[i].dttmarr->init(executeBaton->maxRows);
        break;
      default :
        executeBaton->error = NJSMessages::getErrorMsg(errUnsupportedDatType);
        return; 
        break;
    }
    defines[i].ind = (short*)malloc (sizeof(short)*(executeBaton->maxRows));
    defines[i].len = (DPI_BUFLEN_TYPE *)malloc(sizeof(DPI_BUFLEN_TYPE)*
                                           executeBaton->maxRows);

    executeBaton->dpistmt->define(i+1, defines[i].fetchType,
                 (defines[i].buf) ? defines[i].buf : defines[i].extbuf,
                 defines[i].maxSize, defines[i].ind, defines[i].len);
  }
  executeBaton->dpistmt->fetch(executeBaton->maxRows);
  executeBaton->defines = defines;
  executeBaton->numCols = numCols;
  executeBaton->rowsFetched = executeBaton->dpistmt->rowsFetched();
 
  /* Special processing for datetime, as it is obtained as descriptors */
  for (unsigned int col = 0; col < numCols; col ++ )
  {
    if ( defines[col].dttmarr )
    {
      long double *dblArr = NULL;
      
      defines[col].buf = 
      dblArr = (long double *)malloc ( sizeof ( long double ) *
                                                executeBaton->rowsFetched );

      for ( int row = 0; row < (int) executeBaton->rowsFetched; row ++ )
      {
        dblArr[row] = defines[col].dttmarr->getDateTime (row) * NJS_DAY2MS;
      }
      defines[col].buf = (void *) dblArr;      
      defines[col].dttmarr->release ();
      defines[col].extbuf = NULL; 
    }
  }

}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Execute method
  
   PARAMETERS:
     UV queue work block 
  
   NOTES:
     Handle for result is formed and handed over to JS   
*/
void Connection::Async_AfterExecute(uv_work_t *req)
{
  NanScope();
  
  eBaton *executeBaton = (eBaton*)req->data;
  v8::TryCatch tc;
  Handle<Value> argv[2];
  if(!(executeBaton->error).empty()) 
  {
    argv[0] = v8::Exception::Error(NanNew<v8::String>((executeBaton->error).c_str()));
    argv[1] = NanUndefined();
  }
  else
  {
    argv[0] = NanUndefined();
    Local<Object> result = NanNew<v8::Object>();
    Handle<Value> rowArray;
    switch(executeBaton->st)
    {
      case DpiStmtSelect :
        rowArray = Connection::GetRows(executeBaton);
        if(!(executeBaton->error).empty()) 
        {
          argv[0] = v8::Exception::Error(NanNew<v8::String>((executeBaton->error).c_str()));
          argv[1] = NanUndefined();
          goto exitAsyncAfterExecute;
        }
        result->Set(NanNew<v8::String>("rows"), rowArray);//, v8::ReadOnly); 
        result->Set(NanNew<v8::String>("outBinds"),NanUndefined());
        result->Set(NanNew<v8::String>("rowsAffected"), NanUndefined()); 
        break;
      case DpiStmtBegin :
      case DpiStmtDeclare :
      case DpiStmtCall :
        result->Set(NanNew<v8::String>("rowsAffected"), NanUndefined());
        result->Set(NanNew<v8::String>("outBinds"),Connection::GetOutBinds(executeBaton));//, v8::ReadOnly);
        result->Set(NanNew<v8::String>("rows"), NanUndefined()); 
        break;
      default :
        result->Set(NanNew<v8::String>("rowsAffected"),
                    NanNew<v8::Integer>((unsigned int) executeBaton->rowsAffected));//, v8::ReadOnly);
        result->Set(NanNew<v8::String>("outBinds"),NanUndefined());
        result->Set(NanNew<v8::String>("rows"), NanUndefined()); 
        break;
    }
    argv[1] = result; 
  }
  exitAsyncAfterExecute:
  Local<Function> callback = NanNew(executeBaton->cb);
  delete executeBaton;
  NanMakeCallback( NanGetCurrentContext()->Global(), callback, 2, argv ); 
  if(tc.HasCaught())
  {
    node::FatalException(tc);
  }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Method to populate Rows Array
  
   PARAMETERS:
     eBaton struct 
  
   RETURNS:
     Rows Handle
*/
v8::Handle<v8::Value> Connection::GetRows (eBaton* executeBaton)
{
  NanEscapableScope();
  Handle<Array> rowsArray;
  switch(executeBaton->outFormat)
  {
    case ROWS_ARRAY :
      rowsArray = NanNew<v8::Array>(executeBaton->rowsFetched);
      for(unsigned int i = 0; i < executeBaton->rowsFetched; i++)
      {
        Local<Array> row = NanNew<v8::Array>(executeBaton->numCols);
        for(unsigned int j = 0; j < executeBaton->numCols; j++)
        {
          long double *dblArr = (long double *)executeBaton->defines[j].buf;
          row->Set(j, Connection::GetValue(
                      executeBaton->defines[j].ind[i],
                      executeBaton->defines[j].fetchType,
                      (executeBaton->defines[j].fetchType == DpiTimestampLTZ ) ?
                        (void *) &dblArr[i] :
                        (void *) ((char *)(executeBaton->defines[j].buf) +
                                 ( i * (executeBaton->defines[j].maxSize ))),
                      executeBaton->defines[j].len[i]));
        }
        rowsArray->Set(i, row);
      }
      break;
    case ROWS_OBJECT :
      rowsArray = NanNew<v8::Array>(executeBaton->rowsFetched);
      for(unsigned int i =0 ; i < executeBaton->rowsFetched; i++)
      {
        Local<Object> row = NanNew<v8::Object>();
  
        for(unsigned int j = 0; j < executeBaton->numCols; j++)
        {
          long double *dblArr = (long double * )executeBaton->defines[j].buf;
          row->Set(NanNew<v8::String>(executeBaton->columnNames[j].c_str(),
                               (int) executeBaton->columnNames[j].length()),
                   Connection::GetValue(
                      executeBaton->defines[j].ind[i],
                      executeBaton->defines[j].fetchType,
                      (executeBaton->defines[j].fetchType == DpiTimestampLTZ ) ?
                        (void *) &dblArr[i] :
                        (void *) ((char *)(executeBaton->defines[j].buf) +
                                 ( i * (executeBaton->defines[j].maxSize ))),
                      executeBaton->defines[j].len[i]));

        }
        rowsArray->Set(i, row);
      }
      break;
    default :
      executeBaton->error = NJSMessages::getErrorMsg(errInvalidPropertyValue, "outFormat");
      goto exitGetRows;
      break;
  }
  exitGetRows:
  return NanEscapeScope(rowsArray); 
}

/*****************************************************************************/
/*
   DESCRIPTION
     Method to create handle from C++ value 
  
   PARAMETERS:
     ind  - to validate the data,
     type - data type of the value,
     val  - value,
     len  - length of the value

   RETURNS:
     Handle
*/
v8::Handle<v8::Value> Connection::GetValue ( short ind, unsigned short type, void* val,
                                     DPI_BUFLEN_TYPE len )
{
  NanEscapableScope();
  Handle<Value> value;
  Local<Date> date;
  if(ind != -1)
  {
     switch(type)
     {
       case (dpi::DpiVarChar) :
          value = NanNew<v8::String>((char*)val, len);
        break;
       case (dpi::DpiInteger) :
         value = NanNew<v8::Integer>(*(int*)val);
         break;
      case (dpi::DpiDouble) :
         value = NanNew<v8::Number>(*(double*)val);
         break;
       case (dpi::DpiTimestampLTZ) :
         //date = Date::Cast(*NanNew<v8::Date>( *(long double*)val ));
         date = NanNew<v8::Date>( *(long double*)val );
         value = date;
        break;
       default :
         break;
    }
  }
  else
  {
    value = NanNull();
  }
  return NanEscapeScope(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Method to populate outbinds object/array 
  
   PARAMETERS:
     eBaton struct 
  
   RETURNS:
     Outbinds object/array 
*/
v8::Handle<v8::Value> Connection::GetOutBinds (eBaton* executeBaton)
{
  NanScope();
  if(!executeBaton->binds.empty())
  {
    if( executeBaton->binds[0]->key.empty() ) 
    {
      // Binds as JS array 
      unsigned int outCount = 0;
      for(unsigned int index = 0; index < executeBaton->binds.size(); index++)
      {
        if(executeBaton->binds[index]->isOut)
          outCount++;
      }
      return GetOutBindArray( executeBaton->binds, outCount );
    }
    else
    {
      // Binds as JS object 
      return GetOutBindObject( executeBaton->binds );
    }
  }
  return NanUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Method to populate outbinds array
   
   PARAMETERS:
     Binds , out binds count 
  
   RETURNS:
     Outbinds array 
*/
v8::Handle<v8::Value> Connection::GetOutBindArray ( std::vector<Bind*> binds,
                                            unsigned int outCount )
{
  NanEscapableScope();

  Local<Array> arrayBinds = NanNew<v8::Array>( outCount );

  unsigned int it = 0; 
  for(unsigned int index = 0; index < binds.size(); index++)
  {
    if(binds[index]->isOut)
    {    
      arrayBinds->Set( it, Connection::GetValue( 
                                binds[index]->ind,
                                binds[index]->type,
                            ( binds[index]->type == DpiTimestampLTZ ) ?
                                binds[index]->extvalue : binds[index]->value,
                                binds[index]->len ) ); 
    }    
    it++;
  }
  return NanEscapeScope(arrayBinds);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Method to populate outbinds object
   
   PARAMETERS:
     Binds  
  
   RETURNS:
     Outbinds object 
*/
v8::Handle<v8::Value> Connection::GetOutBindObject ( std::vector<Bind*> binds )
{
  NanEscapableScope();
  Local<Object> objectBinds = NanNew<v8::Object>();
  for(unsigned int index = 0; index < binds.size(); index++)
  {
    if(binds[index]->isOut)
    {
      binds[index]->key.erase(binds[index]->key.begin());
      objectBinds->Set( NanNew<v8::String> ( binds[index]->key.c_str(),
                                      (int) binds[index]->key.length() ),
                        Connection::GetValue( 
                             binds[index]->ind,
                             binds[index]->type,
                         (binds[index]->type == DpiTimestampLTZ ) ?
                             binds[index]->extvalue : binds[index]->value,
                             binds[index]->len ) );
    }
  }
  return NanEscapeScope(objectBinds);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Release method on Connection class.
  
   PARAMETERS:
     Arguments - Callback
*/
NAN_METHOD(Connection::Release)
{
  NanScope();
  Local<Function> callback;
  Connection *connection;
  NJS_GET_CALLBACK ( callback, args );

  eBaton* releaseBaton = new eBaton;
  NanAssignPersistent( releaseBaton->cb, callback ); 
  NJS_CHECK_NUMBER_OF_ARGS ( releaseBaton->error, args, 1, 1, exitRelease );
  connection = ObjectWrap::Unwrap<Connection>(args.This());

  if(!connection->isValid_)
  {
    releaseBaton->error = NJSMessages::getErrorMsg ( errInvalidConnection );
    goto exitRelease;
  }
  connection->isValid_    = false;
  releaseBaton->dpiconn   = connection->dpiconn_;  
  exitRelease: 
  releaseBaton->req.data  = (void*) releaseBaton;

  uv_queue_work(uv_default_loop(), &releaseBaton->req, 
               Async_Release, (uv_after_work_cb)Async_AfterRelease);
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Release method
  
   PARAMETERS:
     UV queue work block 

   NOTES:
     DPI call execution
*/
void Connection::Async_Release(uv_work_t *req)
{
  eBaton *releaseBaton = (eBaton*)req->data;
  if(!(releaseBaton->error).empty()) goto exitAsyncRelease;

  try
  {
    releaseBaton->dpiconn->release();
  }
  catch (dpi::Exception& e)
  {
    releaseBaton->error = std::string(e.what());
  }
  exitAsyncRelease:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Release method
  
   PARAMETERS:
     UV queue work block 
*/
void Connection::Async_AfterRelease(uv_work_t *req)
{
  NanScope();
  eBaton *releaseBaton = (eBaton*)req->data;
  v8::TryCatch tc;

  Handle<Value> argv[1];
 
  if(!(releaseBaton->error).empty()) 
    argv[0] = v8::Exception::Error(NanNew<v8::String>((releaseBaton->error).c_str()));
  else
    argv[0] = NanUndefined();
  Local<Function> callback = NanNew(releaseBaton->cb);
  delete releaseBaton;
  NanMakeCallback( NanGetCurrentContext()->Global(), 
                      callback, 1, argv ); 
  if(tc.HasCaught())
  {
    node::FatalException(tc);
  }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Commit method on Connection class.
  
   PARAMETERS:
     Arguments - Callback
*/
NAN_METHOD(Connection::Commit)
{
  NanScope();
  Local<Function> callback;
  Connection *connection;
  NJS_GET_CALLBACK ( callback, args );

  eBaton* commitBaton = new eBaton;
  NanAssignPersistent( commitBaton->cb, callback ); 
  NJS_CHECK_NUMBER_OF_ARGS ( commitBaton->error, args, 1, 1, exitCommit );
  connection = ObjectWrap::Unwrap<Connection>(args.This());

  if(!connection->isValid_)
  {
    commitBaton->error = NJSMessages::getErrorMsg ( errInvalidConnection );
    goto exitCommit;
  }
  commitBaton->dpiconn   = connection->dpiconn_;
exitCommit:
  commitBaton->req.data  = (void*) commitBaton;

  uv_queue_work(uv_default_loop(), &commitBaton->req, 
               Async_Commit, (uv_after_work_cb)Async_AfterCommit);

  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Commit method
  
   PARAMETERS:
     UV queue work block 

   NOTES:
     DPI call execution     
*/
void Connection::Async_Commit (uv_work_t *req)
{
  eBaton *commitBaton = (eBaton*)req->data;

  if(!(commitBaton->error).empty()) goto exitAsyncCommit;

  try
  {  
    commitBaton->dpiconn->commit();
  }
  catch (dpi::Exception& e)
  {
    commitBaton->error = std::string(e.what());
  }
  exitAsyncCommit:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Commit method
  
   PARAMETERS:
     UV queue work block 
*/
void Connection::Async_AfterCommit (uv_work_t *req)
{
  NanScope();
  eBaton *commitBaton = (eBaton*)req->data;

  v8::TryCatch tc;
  Handle<Value> argv[1];
 
  if(!(commitBaton->error).empty())
    argv[0] = v8::Exception::Error(NanNew<v8::String>((commitBaton->error).c_str()));
  else
    argv[0] = NanUndefined();
   
  NanMakeCallback( NanGetCurrentContext()->Global(), 
                      NanNew(commitBaton->cb), 1, argv ); 
  if(tc.HasCaught())
  {
    node::FatalException(tc);
  }
  delete commitBaton;
}

/*****************************************************************************/
/*
   DESCRIPTION
    Rollback method on Connection class.
  
   PARAMETERS:
     Arguments - Callback
*/
NAN_METHOD(Connection::Rollback)
{
  NanScope();
  Local<Function> callback;
  Connection *connection;
  NJS_GET_CALLBACK ( callback, args );

  eBaton* rollbackBaton = new eBaton;
  NanAssignPersistent( rollbackBaton->cb, callback ); 
  NJS_CHECK_NUMBER_OF_ARGS ( rollbackBaton->error, args, 1, 1, exitRollback );
  connection = ObjectWrap::Unwrap<Connection>(args.This());

  if(!connection->isValid_)
  {
    rollbackBaton->error = NJSMessages::getErrorMsg ( errInvalidConnection );
    goto exitRollback;
  }
  rollbackBaton->dpiconn   = connection->dpiconn_;
  exitRollback: 
  rollbackBaton->req.data  = (void*) rollbackBaton;
  uv_queue_work(uv_default_loop(), &rollbackBaton->req, 
                Async_Rollback, (uv_after_work_cb)Async_AfterRollback);
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Rollback method
  
   PARAMETERS:
     UV queue work block 

   NOTES:
     DPI call execution
*/
void Connection::Async_Rollback (uv_work_t *req)
{
   
  eBaton *rollbackBaton = (eBaton*)req->data;
  if(!(rollbackBaton->error).empty()) goto exitAsyncRollback;
   
  try 
  {  
    rollbackBaton->dpiconn->rollback();
  }
  catch (dpi::Exception& e)
  {
    rollbackBaton->error = std::string(e.what());
  }
  exitAsyncRollback:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Rollback method
  
   PARAMETERS:
     UV queue work block 
*/
void Connection::Async_AfterRollback(uv_work_t *req)
{
  NanScope();
  eBaton *rollbackBaton = (eBaton*)req->data;

  v8::TryCatch tc;
  Handle<Value> argv[1];

  if(!(rollbackBaton->error).empty()) 
    argv[0] = v8::Exception::Error(NanNew<v8::String>((rollbackBaton->error).c_str()));
  else
    argv[0] = NanUndefined();

  NanMakeCallback( NanGetCurrentContext()->Global(), 
                      NanNew(rollbackBaton->cb), 1, argv );
  if(tc.HasCaught())
  {
    node::FatalException(tc);
  }
  delete rollbackBaton;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Break method on Connection class.
  
   PARAMETERS:
     Arguments - Callback
*/
NAN_METHOD(Connection::Break)
{
  NanScope();
  Local<Function> callback;
  Connection *connection;
  NJS_GET_CALLBACK ( callback, args );

  eBaton* breakBaton = new eBaton;
  NanAssignPersistent( breakBaton->cb, callback ); 
  NJS_CHECK_NUMBER_OF_ARGS ( breakBaton->error, args, 1, 1, exitBreak );
  connection = ObjectWrap::Unwrap<Connection>(args.This());

  if(!connection->isValid_)
  {
    breakBaton->error = NJSMessages::getErrorMsg ( errInvalidConnection );
    goto exitBreak;
  }
  breakBaton->dpiconn   = connection->dpiconn_;
  exitBreak: 
  breakBaton->req.data  = (void*) breakBaton;

  uv_queue_work(uv_default_loop(), &breakBaton->req, 
               Async_Break, (uv_after_work_cb)Async_AfterBreak);
 
  NanReturnUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Break method
  
   PARAMETERS:
     UV queue work block 

   NOTES:
     DPI call execution 
*/
void Connection::Async_Break(uv_work_t *req)
{
   
  eBaton *breakBaton = (eBaton*)req->data;
  
  if(!(breakBaton->error).empty()) goto exitAsyncBreak;

  try 
  {  
    breakBaton->dpiconn->breakExecution();
  }
  catch (dpi::Exception& e) 
  {
    breakBaton->error = std::string(e.what());
  }
  exitAsyncBreak:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Break method
  
   PARAMETERS:
     UV queue work block 
*/
void Connection::Async_AfterBreak (uv_work_t *req)
{
  NanScope();
  eBaton *breakBaton = (eBaton*)req->data;

  v8::TryCatch tc;
  Handle<Value> argv[1];

  if(!(breakBaton->error).empty()) 
    argv[0] = v8::Exception::Error(NanNew<v8::String>((breakBaton->error).c_str()));
  else
    argv[0] = NanUndefined();
  NanMakeCallback( NanGetCurrentContext()->Global(), 
                      NanNew(breakBaton->cb), 1, argv );
  if(tc.HasCaught())
  {
    node::FatalException(tc);
  }
  delete breakBaton;
}

/****************************************************************************/
/* NAME
 *   Connection::v8Date2OraDate
 *
 * DESCRIPTION
 *   To convert v8::Date value (double value in seconds) to Oracle DB Type
 *
 * PARAMETERS
 *   val      - expected to be a v8::Date Value
 *   bind     - bind structure to update.
 *
 * NOTE:
 *   This function is used for IN Bind parameters, when v8::Date value is
 *   passed, conversion to Oracle-DB Type happens here.
 *
 */
void Connection::v8Date2OraDate ( Handle<Value> val, Bind *bind)
{
  NanScope();
  Handle<Date> date = val.As<Date>();    // Expects to be of v8::Date type
  
  // Get the number of seconds from 1970-1-1 0:0:0
  *(long double *)(bind->extvalue) = date->NumberValue ();
}
 
/***************************************************************************/
/* NAME
 *   Connection::UpdateDateValue
 *
 * DESCRIPTION
 *   Update the double-date value in Bind structure
 *
 * PARAMETERS
 *   ebaton   - execute Baton
 *
 * NOTE:
 *   When execution process starts, base date is not initialized yet,
 *   Once the stmt object is created and datetimeArray object created, 
 *   conversion can happen.  This funciton is used to convert
 *   Used for IN bind to provide the v8::Date value.
 *
 */
void Connection::UpdateDateValue ( eBaton * ebaton )
{
  for (unsigned int b = 0; b < ebaton->binds.size(); b ++ )
  {
    Bind * bind = ebaton->binds[b];
 
    if ( bind->type == dpi::DpiTimestampLTZ )
    {
      bind->dttmarr = ebaton->dpienv->getDateTimeArray ( 
                                          ebaton->dpistmt->getError () );
      bind->value = bind->dttmarr->init (1);
      if (!bind->isOut)
      {
        bind->dttmarr->setDateTime( 0, 
                                    ((*(long double *)bind->extvalue) /
                                     ( NJS_DAY2MS )));
      }
    }
  }
}
 
 
 

/* end of file njsConnection.cpp */

