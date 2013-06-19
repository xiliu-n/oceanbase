/**
 * (C) 2010-2012 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Version: $Id$
 *
 * ob_tablet_scan.h
 *
 * Authors:
 *   Zhifeng YANG <zhuweng.yzf@taobao.com>
 *
 */
#ifndef _OB_TABLET_SCAN_H
#define _OB_TABLET_SCAN_H 1

#include "sql/ob_sstable_scan.h"
#include "sql/ob_ups_scan.h"
#include "sql/ob_multiple_scan_merge.h"
#include "sql/ob_tablet_direct_join.h"
#include "sql/ob_tablet_cache_join.h"
#include "sql/ob_sql_expression.h"
#include "sql/ob_project.h"
#include "sql/ob_filter.h"
#include "sql/ob_scalar_aggregate.h"
#include "sql/ob_sort.h"
#include "sql/ob_merge_groupby.h"
#include "sql/ob_limit.h"
#include "sql/ob_table_rename.h"
#include "common/ob_schema.h"
#include "common/hash/ob_hashset.h"
#include "sql/ob_sql_scan_param.h"
#include "sql/ob_tablet_read.h"

namespace oceanbase
{
  namespace sql
  {
    namespace test
    {
      class ObTabletScanTest_create_plan_not_join_Test;
      class ObTabletScanTest_create_plan_join_Test;
      class ObTabletScanTest_serialize_Test;
    }

    // 用于CS从磁盘扫描一个tablet，合并、join动态数据，并执行计算过滤等
    // @code
    // int cs_handle_scan(...)
    // {
    //   ObTabletScan tablet_scan_op;
    //   ScanResult results;
    //   设置tablet_scan的参数;
    //   tablet_scan_op.open();
    //   const ObRow *row = NULL;
    //   for(tablet_scan_op.get_next_row(row))
    //   {
    //     results.output(row);
    //   }
    //   tablet_scan_op.close();
    //   send_response(results);
    // }
    class ObTabletScan : public ObTabletRead
    {
      friend class test::ObTabletScanTest_create_plan_not_join_Test;
      friend class test::ObTabletScanTest_create_plan_join_Test;
      friend class test::ObTabletScanTest_serialize_Test;

      struct OpFlag
      {
        uint64_t has_join_ : 1;
        uint64_t has_rename_ : 1;
        uint64_t has_project_ : 1;
        uint64_t has_filter_ : 1;
        uint64_t has_scalar_agg_ : 1;
        uint64_t has_group_ : 1;
        uint64_t has_limit_ : 1;
        uint64_t reserved_ : 57;
      };

      public:
        ObTabletScan();
        virtual ~ObTabletScan();
        
        void reset(void);
        inline int get_tablet_range(ObNewRange& range);

        virtual int create_plan(const ObSchemaManagerV2 &schema_mgr);
        
        bool has_incremental_data() const;
        int64_t to_string(char* buf, const int64_t buf_len) const;
        inline void set_sql_scan_param(const ObSqlScanParam &sql_scan_param);
        void set_scan_context(const ScanContext &scan_context)
        {
          scan_context_ = scan_context;
        }
        inline void set_server_type(common::ObServerType server_type)
        {
          server_type_ = server_type;
        }

      private:
        // disallow copy
        ObTabletScan(const ObTabletScan &other);
        ObTabletScan& operator=(const ObTabletScan &other);

        bool check_inner_stat() const;
        int need_incremental_data(
            const uint64_t *basic_columns,
            const uint64_t basic_column_count,
            ObTabletJoin::TableJoinInfo &table_join_info, 
            const ObRowkeyInfo *right_table_rowkey_info,
            int64_t start_data_version, 
            int64_t end_data_version,
            int64_t rowkey_cell_count);
        int build_sstable_scan_param(
            const uint64_t *basic_columns, 
            const uint64_t count,  
            const int64_t  rowkey_cell_count,
            const ObSqlScanParam &sql_scan_param, 
            sstable::ObSSTableScanParam &sstable_scan_param) const;

      private:
        // data members
        const ObSqlScanParam *sql_scan_param_;
        ScanContext scan_context_;
        ObSSTableScan op_sstable_scan_;
        common::ObServerType server_type_;

        /* operator maybe used */
        ObUpsScan op_ups_scan_;
        ObMultipleScanMerge op_tablet_scan_merge_;
        ObTabletDirectJoin op_tablet_join_;
        ObTableRename op_rename_;
        ObFilter op_filter_;
        ObProject op_project_;
        ObScalarAggregate op_scalar_agg_;
        ObMergeGroupBy op_group_;
        ObSort op_group_columns_sort_;
        ObLimit op_limit_;

        OpFlag op_flag_;
    };

    int ObTabletScan::get_tablet_range(ObNewRange& range) 
    { 
      int ret = OB_SUCCESS;
      ret = op_sstable_scan_.get_tablet_range(range);
      return ret;
    }

    void ObTabletScan::set_sql_scan_param(const ObSqlScanParam &sql_scan_param)
    {
      sql_scan_param_ = &sql_scan_param;
    }
  } // end namespace sql
} // end namespace oceanbase

#endif /* _OB_TABLET_SCAN_H */