/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Asix Class                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_asix.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_asix_endpoints_get                   PORTABLE C      */ 
/*                                                           6.2.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function distinguishes for either the Data or Control Class.   */
/*    For the data class, we mount the bulk in and bulk out endpoints.    */ 
/*    For the control class, we mount the optional interrupt endpoint.    */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    asix                                      Pointer to asix class     */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_interface_endpoint_get     Get interface endpoint    */ 
/*    _ux_host_stack_transfer_request           Transfer request          */
/*    _ux_utility_memory_allocate               Allocate memory           */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    _ux_host_class_asix_activate              Activate asix class       */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  10-15-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            use pre-calculated value    */
/*                                            instead of wMaxPacketSize,  */
/*                                            resulting in version 6.1.9  */
/*  10-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            removed packet RX callback, */
/*                                            refined INT EP start time,  */
/*                                            resulting in version 6.2.0  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_asix_endpoints_get(UX_HOST_CLASS_ASIX *asix)
{

UINT            status;
UINT            endpoint_index;
UX_ENDPOINT     *endpoint;
UX_TRANSFER     *transfer_request;

    
    /* Search the bulk OUT endpoint. It is attached to the interface container.  */
    for (endpoint_index = 0; endpoint_index < asix -> ux_host_class_asix_interface -> ux_interface_descriptor.bNumEndpoints;
                        endpoint_index++)
    {                        
    
        /* Get interface endpoint.  */
        status =  _ux_host_stack_interface_endpoint_get(asix -> ux_host_class_asix_interface, endpoint_index, &endpoint);
    
        /* Check the completion status.  */
        if (status == UX_SUCCESS)
        {
    
            /* Check if endpoint is bulk and OUT.  */
            if (((endpoint -> ux_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == UX_ENDPOINT_OUT) &&
                ((endpoint -> ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_BULK_ENDPOINT))
            {
    
                /* This transfer_request always have the OUT direction.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_type =  UX_REQUEST_OUT;


                /* There is a callback function associated with the transfer request, so we need the class instance.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_class_instance =  (VOID *) asix;
                
                /* The transfer request has a callback function.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_completion_function =  _ux_host_class_asix_transmission_callback;

                /* We have found the bulk endpoint, save it.  */
                asix -> ux_host_class_asix_bulk_out_endpoint =  endpoint;
                break;
            }
        }                
    }            
    
    /* The bulk out endpoint is mandatory.  */
    if (asix -> ux_host_class_asix_bulk_out_endpoint == UX_NULL)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, asix, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);
    }
        
    /* Search the bulk IN endpoint. It is attached to the interface container.  */
    for (endpoint_index = 0; endpoint_index < asix -> ux_host_class_asix_interface -> ux_interface_descriptor.bNumEndpoints;
                        endpoint_index++)
    {                        
    
        /* Get the endpoint handle.  */
        status =  _ux_host_stack_interface_endpoint_get(asix -> ux_host_class_asix_interface, endpoint_index, &endpoint);
    
        /* Check the completion status.  */
        if (status == UX_SUCCESS)
        {
    
            /* Check if endpoint is bulk and IN.  */
            if (((endpoint -> ux_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == UX_ENDPOINT_IN) &&
                ((endpoint -> ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_BULK_ENDPOINT))
            {
    
                /* This transfer_request always have the IN direction.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_type =  UX_REQUEST_IN;

                /* There is a callback function associated with the transfer request, so we need the class instance.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_class_instance =  (VOID *) asix;

                /* We have found the bulk endpoint, save it.  */
                asix -> ux_host_class_asix_bulk_in_endpoint =  endpoint;
                break;
            }
        }                
    }    

    /* The bulk in endpoint is mandatory.  */
    if (asix -> ux_host_class_asix_bulk_in_endpoint == UX_NULL)
    {
    
        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, asix, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);
    }
    
    /* Search the Interrupt endpoint. It is mandatory.  */
    for (endpoint_index = 0; endpoint_index < asix -> ux_host_class_asix_interface -> ux_interface_descriptor.bNumEndpoints;
                        endpoint_index++)
    {                        
    
        /* Get the endpoint handle.  */
        status =  _ux_host_stack_interface_endpoint_get(asix -> ux_host_class_asix_interface, endpoint_index, &endpoint);
    
        /* Check the completion status.  */
        if (status == UX_SUCCESS)
        {
    
            /* Check if endpoint is Interrupt and IN.  */
            if (((endpoint -> ux_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == UX_ENDPOINT_IN) &&
                ((endpoint -> ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_INTERRUPT_ENDPOINT))
            {
    
                /* This transfer_request always have the IN direction.  */
                endpoint -> ux_endpoint_transfer_request.ux_transfer_request_type =  UX_REQUEST_IN;
    
                /* We have found the interrupt endpoint, save it.  */
                asix -> ux_host_class_asix_interrupt_endpoint =  endpoint;

                /* The endpoint is correct, Fill in the transfer request with the length requested for this endpoint.  */
                transfer_request =  &asix -> ux_host_class_asix_interrupt_endpoint -> ux_endpoint_transfer_request;
                transfer_request -> ux_transfer_request_requested_length =  transfer_request -> ux_transfer_request_packet_length;
                transfer_request -> ux_transfer_request_actual_length =     0;

                /* The direction is always IN for the CDC interrupt endpoint.  */
                transfer_request -> ux_transfer_request_type =  UX_REQUEST_IN;

                /* There is a callback function associated with the transfer request, so we need the class instance.  */
                transfer_request -> ux_transfer_request_class_instance =  (VOID *) asix;

                /* Interrupt transactions have a completion routine. */
                transfer_request -> ux_transfer_request_completion_function =  _ux_host_class_asix_interrupt_notification;

                /* Obtain a buffer for this transaction. The buffer will always be reused.  */
                transfer_request -> ux_transfer_request_data_pointer =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, 
                                                                transfer_request -> ux_transfer_request_requested_length);

                /* If the endpoint is available and we have memory, it's started later after setup.  */
                if (transfer_request -> ux_transfer_request_data_pointer == UX_NULL)
                {

                    /* Error trap. */
                    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_MEMORY_INSUFFICIENT);

                    /* If trace is enabled, insert this event into the trace buffer.  */
                    UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_MEMORY_INSUFFICIENT, endpoint, 0, 0, UX_TRACE_ERRORS, 0, 0)

                    /* We must return an error.  */
                    return(UX_ENDPOINT_HANDLE_UNKNOWN);
                }
                            
                break;
            }
        }                
    }    

    /* The interrupt endpoint is mandatory.  */
    if (asix -> ux_host_class_asix_interrupt_endpoint == UX_NULL)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, asix, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);
    }
    else    
    
        /* All endpoints have been mounted.  */
        return(UX_SUCCESS);
}

