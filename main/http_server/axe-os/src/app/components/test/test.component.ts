import { HttpErrorResponse, HttpEventType } from '@angular/common/http';
import { Component, OnDestroy, OnInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { ToastrService } from 'ngx-toastr';
import { FileUploadHandlerEvent } from 'primeng/fileupload';
import { map, Observable, startWith } from 'rxjs';
import { LoadingService } from 'src/app/services/loading.service';
import { SystemService } from 'src/app/services/system.service';
import { eASICModel } from 'src/models/enum/eASICModel';

@Component({
  selector: 'app-test',
  templateUrl: './test.component.html',
  styleUrls: ['./test.component.scss'],
})
export class TestComponent {
  public form!: FormGroup;

  public firmwareUpdateProgress: number | null = null;
  public websiteUpdateProgress: number | null = null;
  public minerModel: string = 'LV07';
  public chips: number | null = 0;
  public selftest: number | null = 0;

  private intervalId: any;
  public eASICModel = eASICModel;
  public ASICModel!: eASICModel;

  constructor(
    private fb: FormBuilder,
    private systemService: SystemService,
    private toastr: ToastrService,
    private toastrService: ToastrService,
    private loadingService: LoadingService
  ) {
    
  }

  ngOnInit(): void {
    this.fetchInfo();
    this.intervalId = setInterval(() => {
      this.fetchInfo();
    }, 5000); // 5000ms = 5s
  }


  public fetchInfo() {
    this.systemService
      .getInfo()
      .pipe()
      .subscribe((info: any) => {
        this.minerModel = info.minerModel;
        this.chips = info.asicDetected;
      });
  }

  // public fetchChips() {
  //   this.systemService
  //     .getUptime('t=&h=')
  //     .pipe()
  //     .subscribe((info: any) => {
  //       this.chips = info.chips;
  //     });
  // }

  public updateSystem(type: string) {
    let form = {
      tps546Off: type,
    };

    this.systemService
      .updateSystem(undefined, form)
      .pipe(this.loadingService.lockUIUntilComplete())
      .subscribe({
        next: () => {
          this.toastr.success('Success!', '');
        },
        error: (err: HttpErrorResponse) => {
          this.toastr.error('Error.', `${err.message}`);
        },
      });
  }

  public updateSystemMode(type: number) {
    let form = {
      selftest: type,
    };

    this.systemService
      .updateSystem(undefined, form)
      .pipe(this.loadingService.lockUIUntilComplete())
      .subscribe({
        next: () => {
          if (type == 1) {
            this.toastr.success('Success!', '开启调试模式，30秒后检测芯片');
          } else {
            this.toastr.success('Success!', '关闭调试模式成功');
          }

          this.systemService.restart().subscribe((res) => {});
        },
        error: (err: HttpErrorResponse) => {
          this.toastr.error('Error.', `${err.message}`);
        },
      });
  }

  ngOnDestroy(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
    }
  }
}
