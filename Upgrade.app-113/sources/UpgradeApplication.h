#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIPushButton.h>
#import <UIKit/UITableCell.h>
#import <UIKit/UIImageAndTextTableCell.h>
#import <UIKit/UISwitchControl.h>
#import <UIKit/UIProgressHUD.h>

@interface UpgradeApplication : UIApplication {
	UIProgressHUD *progress;
	int theButtonClicked;
	BOOL hactivation;
}

- (void)setProgressHUDText:(NSString *) label;
- (void)setProgressHUDTextMain:(id) label;
- (void)showProgressHUD:(NSString *)label withWindow:(UIWindow *)w withView:(UIView *)v withRect:(struct CGRect)rect;
- (void)hideProgressHUD:(id) anObject;
- (void)jailbreak:(id)param;
- (void)doProgress: (unsigned int)progressBytes withTotal:(unsigned int)totalBytes withFormat:(char*)formatString;

- (void)alertSheet:(UIAlertSheet*)sheet buttonClicked:(int)button;
- (void)displayAlert:(NSString*)alert withTitle: (NSString*) title;
- (BOOL)displayAlertQuestion:(NSString*)alert withTitle: (NSString*) title;
void LOGDEBUG (const char *err, ...) ;
@end
