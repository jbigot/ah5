static inline int execute_write( ah5_t self, command_write *cmd )
{
	LOG_DEBUG("async HDF5 writing data: %s of rank %u", cmd->name, 
			(unsigned)cmd->rank);
	hid_t space_id = H5Screate_simple(cmd->rank, cmd->dims, NULL);
	hid_t plist_id = H5Pcreate(CLS_DSET_CREATE);
	if ( H5Pset_layout(plist_id, H5D_CONTIGUOUS) ) RETURN_ERROR;
#if ( H5Dcreate_vers == 2 )
	hid_t dset_id = H5Dcreate2(self->file_id, cmd->name, cmd->type, space_id, 
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
#else
	hid_t dset_id = H5Dcreate(self->file_id, cmd->name, cmd->type, space_id, 
			H5P_DEFAULT);
#endif
	if ( H5Dwrite(dset_id, cmd->type, H5S_ALL, H5S_ALL, H5P_DEFAULT, cmd->buf) ) {
		RETURN_ERROR;
	}
	if ( H5Dclose(dset_id) ) RETURN_ERROR;
	if ( H5Pclose(plist_id) ) RETURN_ERROR;
	if ( H5Sclose(space_id) ) RETURN_ERROR;
	return 0;
}



static inline int execute_close( command_queue_t *self )
{
	LOG_DEBUG("async HDF5 closing file");
	if ( H5Fclose(self->file_id) ) RETURN_ERROR;
	self->file_id = 0;
	return 0;
}


